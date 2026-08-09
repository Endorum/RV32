// CPU execution tests — run tiny hand-assembled programs through the public
// API (attach_bus / reset / step) against a private TestRAM device, then read
// results back through the bus. Same observation model as riscv-tests: the
// program stores what it computed, the test checks memory.
//
// Memory layout: program at 0x0 (reset pc), results at RESULT0/1/2.

#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "../include/CPU.hpp"

#include "test_common.hpp"

static constexpr u32 RAM_BYTES = 0x1000;
static constexpr u32 RESULT0 = 0x200;
static constexpr u32 RESULT1 = 0x204;
static constexpr u32 RESULT2 = 0x208;

// minimal RAM so these tests don't depend on RAM.hpp
class TestRAM : public Device {
public:
  u32 load(u32 address, BITSIZE size) override {
    u32 v = 0;
    for (u32 i = 0; i < (u32)size; i++)
      v |= (u32)mem[address + i] << (8 * i);
    return v;
  }

  void store(u32 address, BITSIZE size, u32 value) override {
    for (u32 i = 0; i < (u32)size; i++)
      mem[address + i] = (u8)(value >> (8 * i));
  }

  std::string str() const override { return "TestRAM"; }

private:
  u8 mem[RAM_BYTES] = {};
};

// load a program at 0x0 into `bus`, then run `steps` instructions.
// (bus is a parameter instead of a return value: Bus declares a destructor,
// which suppresses its move constructor, and unique_ptr forbids copies)
static void run_program(Bus& bus, const std::vector<u32>& prog, int steps) {
  {
    // swallow addDevice's "Adding Device:" chatter
    std::ostringstream sink;
    std::streambuf* old = std::cout.rdbuf(sink.rdbuf());
    bus.addDevice("TestRAM", 0x0, RAM_BYTES, std::make_unique<TestRAM>());
    std::cout.rdbuf(old);
  }

  for (u32 i = 0; i < prog.size(); i++)
    bus.store(i * 4, WORD, prog[i]);

  CPU cpu;
  cpu.attach_bus(&bus);
  cpu.reset(); // pc = 0
  for (int i = 0; i < steps; i++)
    cpu.step();
}

// run a test body; a stray exception (bad jump target, invalid decode, ...)
// counts as a failure instead of killing the whole run. Error<>'s cerr print
// is swallowed — the exception message is reported instead.
template <typename Fn>
static void guarded(const char* name, Fn fn) {
  std::ostringstream sink;
  std::streambuf* old = std::cerr.rdbuf(sink.rdbuf());
  try {
    fn();
  } catch (const std::exception& e) {
    checks++;
    failures++;
    std::printf("FAIL [%s]  unexpected exception: %s\n", name, e.what());
  }
  std::cerr.rdbuf(old);
}

// ------------------------------------------------------------ ALU basics

static void test_add() {
  guarded("add", [] {
    Bus bus;
    run_program(bus, {
      0x00500293, // addi t0, x0, 5
      0x00700313, // addi t1, x0, 7
      0x006283B3, // add  t2, t0, t1
      0x20702023, // sw   t2, 0x200(x0)
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 12);
  });
}

static void test_sub_negative_and_sra() {
  guarded("sub/sra", [] {
    Bus bus;
    run_program(bus, {
      0x00500293, // addi t0, x0, 5
      0x00700313, // addi t1, x0, 7
      0x406283B3, // sub  t2, t0, t1        -> -2
      0x4013DE13, // srai t3, t2, 1         -> -1 (sign bit replicated)
      0x20702023, // sw   t2, 0x200(x0)
      0x21C02223, // sw   t3, 0x204(x0)
    }, 6);
    CHECK_EQ(bus.load(RESULT0, WORD), 0xFFFFFFFE);
    CHECK_EQ(bus.load(RESULT1, WORD), 0xFFFFFFFF);
  });
}

static void test_slt_signed_vs_unsigned() {
  guarded("slt/sltu", [] {
    Bus bus;
    run_program(bus, {
      0xFFF00293, // addi t0, x0, -1
      0x00100313, // addi t1, x0, 1
      0x0062A3B3, // slt  t2, t0, t1        -> 1  (-1 < 1 signed)
      0x0062BE33, // sltu t3, t0, t1        -> 0  (0xFFFFFFFF > 1 unsigned)
      0x20702023, // sw   t2, 0x200(x0)
      0x21C02223, // sw   t3, 0x204(x0)
    }, 6);
    CHECK_EQ(bus.load(RESULT0, WORD), 1);
    CHECK_EQ(bus.load(RESULT1, WORD), 0);
  });
}

static void test_shift_amount_masked() {
  guarded("sll masking", [] {
    Bus bus;
    run_program(bus, {
      0x00100293, // addi t0, x0, 1
      0x02100313, // addi t1, x0, 33
      0x006293B3, // sll  t2, t0, t1        -> 1 << (33 & 31) = 2
      0x20702023, // sw   t2, 0x200(x0)
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 2);
  });
}

// -------------------------------------------------------- load extension

static void test_lb_sign_extension() {
  guarded("lb/lbu", [] {
    Bus bus;
    run_program(bus, {
      0xF8000293, // addi t0, x0, -128
      0x20500023, // sb   t0, 0x200(x0)     memory byte = 0x80
      0x20000303, // lb   t1, 0x200(x0)     -> 0xFFFFFF80 (sign-extended)
      0x20004383, // lbu  t2, 0x200(x0)     -> 0x00000080 (zero-extended)
      0x20602223, // sw   t1, 0x204(x0)
      0x20702423, // sw   t2, 0x208(x0)
    }, 6);
    CHECK_EQ(bus.load(RESULT1, WORD), 0xFFFFFF80);
    CHECK_EQ(bus.load(RESULT2, WORD), 0x00000080);
  });
}

static void test_lh_sign_extension() {
  guarded("lh/lhu", [] {
    Bus bus;
    run_program(bus, {
      0xFFE00293, // addi t0, x0, -2
      0x20501023, // sh   t0, 0x200(x0)     memory half = 0xFFFE
      0x20001303, // lh   t1, 0x200(x0)     -> 0xFFFFFFFE
      0x20005383, // lhu  t2, 0x200(x0)     -> 0x0000FFFE
      0x20602223, // sw   t1, 0x204(x0)
      0x20702423, // sw   t2, 0x208(x0)
    }, 6);
    CHECK_EQ(bus.load(RESULT1, WORD), 0xFFFFFFFE);
    CHECK_EQ(bus.load(RESULT2, WORD), 0x0000FFFE);
  });
}

static void test_sb_touches_one_byte() {
  guarded("sb width", [] {
    Bus bus;
    run_program(bus, {
      0xFFF00293, // addi t0, x0, -1        t0 = 0xFFFFFFFF
      0x20502023, // sw   t0, 0x200(x0)     word = FFFFFFFF
      0x00000313, // addi t1, x0, 0
      0x20600023, // sb   t1, 0x200(x0)     only lowest byte cleared
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 0xFFFFFF00);
  });
}

// ------------------------------------------------------------- branches

static void test_branch_taken() {
  guarded("beq taken", [] {
    Bus bus;
    run_program(bus, {
      0x00100293, // 0x00: addi t0, x0, 1
      0x00100313, // 0x04: addi t1, x0, 1
      0x00628663, // 0x08: beq  t0, t1, +12  -> 0x14
      0x06F00393, // 0x0C: addi t2, x0, 111  (must be skipped)
      0x0080006F, // 0x10: jal  x0, +8       (not-taken detector -> 0x18)
      0x02A00393, // 0x14: addi t2, x0, 42
      0x20702023, // 0x18: sw   t2, 0x200(x0)
    }, 5);
    // 42 = correct. 111 = branch not taken. 0 = wrong branch target.
    CHECK_EQ(bus.load(RESULT0, WORD), 42);
  });
}

static void test_branch_not_taken() {
  guarded("bne not taken", [] {
    Bus bus;
    run_program(bus, {
      0x00100293, // 0x00: addi t0, x0, 1
      0x00529463, // 0x04: bne  t0, t0, +8   (equal -> must NOT branch)
      0x00700393, // 0x08: addi t2, x0, 7
      0x20702023, // 0x0C: sw   t2, 0x200(x0)
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 7);
  });
}

// ---------------------------------------------------------------- jumps

static void test_jal_link_and_target() {
  guarded("jal", [] {
    Bus bus;
    run_program(bus, {
      0x008000EF, // 0x00: jal  ra, +8       -> 0x08, ra = 0x04
      0x06F00393, // 0x04: addi t2, x0, 111  (must be skipped)
      0x20102023, // 0x08: sw   ra, 0x200(x0)
    }, 2);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x04);
  });
}

static void test_jalr_target_masked_and_link() {
  guarded("jalr", [] {
    Bus bus;
    run_program(bus, {
      0x01000293, // 0x00: addi t0, x0, 0x10
      0x001280E7, // 0x04: jalr ra, 1(t0)    target (0x10+1)&~1 = 0x10, ra = 0x08
      0x06F00393, // 0x08: addi t2, x0, 111  (must be skipped)
      0x06F00393, // 0x0C: addi t2, x0, 111  (must be skipped)
      0x20102023, // 0x10: sw   ra, 0x200(x0)
    }, 3);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x08);
  });
}

// ---------------------------------------------------------- LUI / AUIPC

static void test_lui_auipc() {
  guarded("lui/auipc", [] {
    Bus bus;
    run_program(bus, {
      0x123452B7, // 0x00: lui   t0, 0x12345  -> 0x12345000
      0x00001317, // 0x04: auipc t1, 1        -> 0x04 + 0x1000 = 0x1004
      0x20502023, // 0x08: sw    t0, 0x200(x0)
      0x20602223, // 0x0C: sw    t1, 0x204(x0)
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x12345000);
    CHECK_EQ(bus.load(RESULT1, WORD), 0x00001004);
  });
}

// ------------------------------------------------------------------- x0

static void test_x0_stays_zero() {
  guarded("x0", [] {
    Bus bus;
    run_program(bus, {
      0x00500013, // addi x0, x0, 5          (write to x0 must vanish)
      0x20002023, // sw   x0, 0x200(x0)
    }, 2);
    CHECK_EQ(bus.load(RESULT0, WORD), 0);
  });
}

// ---------------------------------------------------------------- entry

void run_cpu_tests() {
  test_add();
  test_sub_negative_and_sra();
  test_slt_signed_vs_unsigned();
  test_shift_amount_masked();
  test_lb_sign_extension();
  test_lh_sign_extension();
  test_sb_touches_one_byte();
  test_branch_taken();
  test_branch_not_taken();
  test_jal_link_and_target();
  test_jalr_target_masked_and_link();
  test_lui_auipc();
  test_x0_stays_zero();
}
