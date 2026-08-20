// UART (16550 subset) device tests — register-level checks against the
// Device interface. Offsets are device-relative, exactly as BUS routes them.
//
// Observation model: TX is captured by swapping std::cout's streambuf for an
// ostringstream; RX is injected through push_rx — the same seam the host
// poller uses — so no terminal is involved. UART::tick() polls the real
// stdin and stays untested here on purpose.

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

#include "../include/BUS.hpp"
#include "../include/MAP.hpp"
#include "../include/devices/UART.hpp"

#include "test_common.hpp"


// LSR bits: DR = receive data ready; THRE|TEMT = transmitter idle, which
// this UART reports permanently (host stdout is infinitely fast)
static constexpr u32 LSR_DR = 0x01;
static constexpr u32 LSR_IDLE = 0x60;

// same guard as in test_cpu.cpp: a stray exception fails the test instead of
// killing the run
template <typename Fn>
static void guarded(const char* name, Fn fn) {
  try {
    fn();
  } catch (const std::exception& e) {
    checks++;
    failures++;
    std::printf("FAIL [%s]  unexpected exception: %s\n", name, e.what());
  }
}

// collect everything the UART prints while `fn` runs
template <typename Fn>
static std::string capture_tx(Fn fn) {
  std::ostringstream sink;
  std::streambuf* old = std::cout.rdbuf(sink.rdbuf());
  fn();
  std::cout.rdbuf(old);
  return sink.str();
}

// ------------------------------------------------------------ status + RX

static void test_lsr_reflects_rx_state() {
  guarded("lsr dr bit", [] {
    UART u{0, 0x100};
    CHECK_EQ(u.load(LSR_ADDR, BYTE), LSR_IDLE);          // empty: DR clear
    u.push_rx('A');
    CHECK_EQ(u.load(LSR_ADDR, BYTE), LSR_IDLE | LSR_DR); // byte pending
    CHECK_EQ(u.load(RBR_ADDR, BYTE), 'A');               // RBR pops it
    CHECK_EQ(u.load(LSR_ADDR, BYTE), LSR_IDLE);          // DR clear again
  });
}

static void test_rbr_fifo_order_and_empty_read() {
  guarded("rbr fifo", [] {
    UART u{0, 0x100};
    CHECK_EQ(u.load(RBR_ADDR, BYTE), 0); // empty read: defined 0, no crash
    u.push_rx('a');
    u.push_rx('b');
    u.push_rx('c');
    CHECK_EQ(u.load(RBR_ADDR, BYTE), 'a'); // FIFO: oldest byte first
    CHECK_EQ(u.load(RBR_ADDR, BYTE), 'b');
    CHECK_EQ(u.load(RBR_ADDR, BYTE), 'c');
    CHECK_EQ(u.load(RBR_ADDR, BYTE), 0);   // drained
  });
}

// ------------------------------------------------------------ TX

static void test_thr_writes_reach_host() {
  guarded("thr tx", [] {
    UART u{0, 0x100};
    std::string out = capture_tx([&] {
      u.store(THR_ADDR, BYTE, 'H');
      u.store(THR_ADDR, BYTE, 'i');
    });
    CHECK(out == "Hi");
  });
}

// ------------------------------------------------------------ FCR

static void test_fcr_clear_bits() {
  guarded("fcr bits", [] {
    UART u{0, 0x100};
    u.push_rx('x');
    u.push_rx('y');
    u.store(FCR_ADDR, BYTE, 0x01);                 // bit0 = FIFO enable ONLY
    CHECK_EQ(u.load(LSR_ADDR, BYTE) & LSR_DR, 1);  // must NOT clear RX
    u.store(FCR_ADDR, BYTE, 0x02);                 // bit1 = RX clear
    CHECK_EQ(u.load(LSR_ADDR, BYTE), LSR_IDLE);    // input gone
    u.store(FCR_ADDR, BYTE, 0x04);                 // bit2 = TX clear: no-op
  });
}

// ------------------------------------------------------------ DLAB

static void test_dlab_bank_switch() {
  guarded("dlab", [] {
    UART u{0, 0x100};
    u.push_rx('k');                      // pending input must survive all of this
    u.store(IER_ADDR, BYTE, 0x05);       // IER while DLAB off
    u.store(LCR_ADDR, BYTE, 0x80);       // DLAB on
    std::string out = capture_tx([&] {
      u.store(0x00, BYTE, 0x03);         // DLL — must not print
      u.store(0x01, BYTE, 0x42);         // DLM — must not touch IER
    });
    CHECK(out.empty());
    CHECK_EQ(u.load(0x00, BYTE), 0x03);  // DLL reads back, must NOT pop rx
    CHECK_EQ(u.load(0x01, BYTE), 0x42);  // DLM reads back
    u.store(LCR_ADDR, BYTE, 0x03);       // DLAB off, 8N1
    CHECK_EQ(u.load(LCR_ADDR, BYTE), 0x03);
    CHECK_EQ(u.load(IER_ADDR, BYTE), 0x05);       // IER survived
    CHECK_EQ(u.load(LSR_ADDR, BYTE) & LSR_DR, 1); // rx survived
    CHECK_EQ(u.load(RBR_ADDR, BYTE), 'k');        // and is still first in line
  });
}

static void test_xv6_init_sequence_is_silent() {
  // the canonical 16550 driver init must produce no boot garbage on the
  // console and leave a working, empty UART behind
  guarded("xv6 init", [] {
    UART u{0, 0x100};
    std::string out = capture_tx([&] {
      u.store(IER_ADDR, BYTE, 0x00); // interrupts off
      u.store(LCR_ADDR, BYTE, 0x80); // DLAB on
      u.store(0x00, BYTE, 0x03);     // DLL = 3 (38400 baud)
      u.store(0x01, BYTE, 0x00);     // DLM = 0
      u.store(LCR_ADDR, BYTE, 0x03); // 8N1, DLAB off
      u.store(FCR_ADDR, BYTE, 0x07); // FIFOs on + clear both
    });
    CHECK(out.empty());
    CHECK_EQ(u.load(LSR_ADDR, BYTE), LSR_IDLE);
    out = capture_tx([&] {
      u.store(THR_ADDR, BYTE, 'O');
      u.store(THR_ADDR, BYTE, 'K');
    });
    CHECK(out == "OK");
  });
}

// ------------------------------------------------------------ access width + bus

static void test_non_byte_access_is_ignored() {
  guarded("word access", [] {
    UART u{0, 0x100};
    u.push_rx('q');
    CHECK_EQ(u.load(RBR_ADDR, WORD), 0);          // word read: must not pop
    u.store(FCR_ADDR, WORD, 0x02);                // word write: must not clear
    CHECK_EQ(u.load(LSR_ADDR, BYTE) & LSR_DR, 1);
    CHECK_EQ(u.load(RBR_ADDR, BYTE), 'q');
  });
}

static void test_bus_routing() {
  guarded("bus routing", [] {
    BUS bus;
    UART u{UART_START, UART_SIZE};
    {
      // swallow addDevice's "Adding Device:" chatter
      std::ostringstream sink;
      std::streambuf* old = std::cout.rdbuf(sink.rdbuf());
      bus.addDevice(u);
      std::cout.rdbuf(old);
    }
    u.push_rx('R');
    CHECK_EQ(bus.load(UART_START + LSR_ADDR, BYTE) & LSR_DR, 1);
    CHECK_EQ(bus.load(UART_START + RBR_ADDR, BYTE), 'R');
    std::string out = capture_tx([&] { bus.store(UART_START + THR_ADDR, BYTE, '!'); });
    CHECK(out == "!");
  });
}

void run_uart_tests() {
  test_lsr_reflects_rx_state();
  test_rbr_fifo_order_and_empty_read();
  test_thr_writes_reach_host();
  test_fcr_clear_bits();
  test_dlab_bank_switch();
  test_xv6_init_sequence_is_silent();
  test_non_byte_access_is_ignored();
  test_bus_routing();
}
