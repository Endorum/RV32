// CPU execution tests — run tiny hand-assembled programs through the public
// API (attach_bus / reset / step) against a private TestRAM device, then read
// results back through the bus. Same observation model as riscv-tests: the
// program stores what it computed, the test checks memory.
//
// Memory layout: program at 0x0 (reset pc), results at RESULT0/1/2.

#include <exception>
#include <iostream>
#include <sstream>
#include <vector>

#include "../include/CPU.hpp"

#include "test_common.hpp"

// BITSIZE became an enum class (2026-08-16); this alias keeps the many
// bus.load(..., WORD) call sites readable
static constexpr BITSIZE WORD = BITSIZE::WORD;

static constexpr u32 RAM_BYTES = 0x1000;
static constexpr u32 RESULT0 = 0x200;
static constexpr u32 RESULT1 = 0x204;
static constexpr u32 RESULT2 = 0x208;

// minimal RAM so these tests don't depend on RAM.hpp
class TestRAM : public Device {
public:
  TestRAM() : Device(0x0, RAM_BYTES, "TestRAM") {}

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

private:
  u8 mem[RAM_BYTES] = {};
};

// load a program at 0x0 into `bus`, then run `steps` instructions.
// The bus holds non-owning pointers now, so `ram` lives in the test body
// alongside `bus` — it must outlive the bus.load() checks after we return.
static void run_program(BUS& bus, TestRAM& ram, const std::vector<u32>& prog, int steps) {
  {
    // swallow addDevice's "Adding Device:" chatter
    std::ostringstream sink;
    std::streambuf* old = std::cout.rdbuf(sink.rdbuf());
    bus.addDevice(ram);
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x008000EF, // 0x00: jal  ra, +8       -> 0x08, ra = 0x04
      0x06F00393, // 0x04: addi t2, x0, 111  (must be skipped)
      0x20102023, // 0x08: sw   ra, 0x200(x0)
    }, 2);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x04);
  });
}

static void test_jalr_target_masked_and_link() {
  guarded("jalr", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
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
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x00500013, // addi x0, x0, 5          (write to x0 must vanish)
      0x20002023, // sw   x0, 0x200(x0)
    }, 2);
    CHECK_EQ(bus.load(RESULT0, WORD), 0);
  });
}

// ------------------------------------------------------------ Zicsr
//
// mscratch (0x340) is used as the guinea-pig CSR: architecturally it's a
// plain read/write scratch register with no side effects.
//
// Not tested (not observable with the flat csr[] array): that csrrs/csrrc
// with rs1=x0 performs NO write. "Writes the old value back" and "doesn't
// write" look identical until read-only CSRs trap on write — pin it then.

static void test_csrrw_swap() {
  guarded("csrrw swap", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x11100293, // addi  t0, x0, 0x111
      0x34029073, // csrw  mscratch, t0        mscratch = 0x111
      0x22200313, // addi  t1, x0, 0x222
      0x340313F3, // csrrw t2, mscratch, t1    t2 = old (0x111), mscratch = 0x222
      0x34002E73, // csrr  t3, mscratch        t3 = 0x222
      0x20702023, // sw    t2, 0x200(x0)
      0x21C02223, // sw    t3, 0x204(x0)
    }, 7);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x111); // rd got the OLD value
    CHECK_EQ(bus.load(RESULT1, WORD), 0x222); // CSR got rs1
  });
}

static void test_csrrs_sets_bits() {
  guarded("csrrs", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x0F000293, // addi  t0, x0, 0xF0
      0x34029073, // csrw  mscratch, t0        mscratch = 0xF0
      0x00F00313, // addi  t1, x0, 0x0F
      0x340323F3, // csrrs t2, mscratch, t1    t2 = 0xF0, mscratch |= 0x0F
      0x34002E73, // csrr  t3, mscratch
      0x20702023, // sw    t2, 0x200(x0)
      0x21C02223, // sw    t3, 0x204(x0)
    }, 7);
    CHECK_EQ(bus.load(RESULT0, WORD), 0xF0);
    CHECK_EQ(bus.load(RESULT1, WORD), 0xFF);
  });
}

static void test_csrrc_clears_bits() {
  guarded("csrrc", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x0FF00293, // addi  t0, x0, 0xFF
      0x34029073, // csrw  mscratch, t0        mscratch = 0xFF
      0x00F00313, // addi  t1, x0, 0x0F
      0x340333F3, // csrrc t2, mscratch, t1    t2 = 0xFF, mscratch &= ~0x0F
      0x34002E73, // csrr  t3, mscratch
      0x20702023, // sw    t2, 0x200(x0)
      0x21C02223, // sw    t3, 0x204(x0)
    }, 7);
    CHECK_EQ(bus.load(RESULT0, WORD), 0xFF);
    CHECK_EQ(bus.load(RESULT1, WORD), 0xF0);
  });
}

static void test_csrrw_x0_still_writes() {
  // asymmetry: rs1=x0 suppresses the write for csrrs/csrrc, but csrrw
  // ALWAYS writes — here it must zero the CSR
  guarded("csrrw x0 writes", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x12300293, // addi  t0, x0, 0x123
      0x34029073, // csrw  mscratch, t0        mscratch = 0x123
      0x34001373, // csrrw t1, mscratch, x0    t1 = 0x123, mscratch = 0
      0x34002E73, // csrr  t3, mscratch        t3 = 0
      0x20602023, // sw    t1, 0x200(x0)
      0x21C02223, // sw    t3, 0x204(x0)
    }, 6);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x123);
    CHECK_EQ(bus.load(RESULT1, WORD), 0);
  });
}

static void test_csrrwi_uses_field_not_register() {
  // uimm = 5 is also the index of t0, which holds a sentinel: an
  // implementation that wrongly reads regfile[uimm] writes 0x7FF instead of 5
  guarded("csrrwi field", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x7FF00293, // addi   t0, x0, 0x7FF      sentinel in x5
      0x3402D073, // csrrwi x0, mscratch, 5    mscratch = 5 (the field itself)
      0x34002373, // csrr   t1, mscratch
      0x20602023, // sw     t1, 0x200(x0)
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 5);
  });
}

static void test_csrrsi_csrrci() {
  guarded("csrrsi/csrrci", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x34085073, // csrrwi x0, mscratch, 16   mscratch = 0x10
      0x3401E373, // csrrsi t1, mscratch, 3    t1 = 0x10, mscratch = 0x13
      0x340173F3, // csrrci t2, mscratch, 2    t2 = 0x13, mscratch = 0x11
      0x34002E73, // csrr   t3, mscratch
      0x20602023, // sw     t1, 0x200(x0)
      0x20702223, // sw     t2, 0x204(x0)
      0x21C02423, // sw     t3, 0x208(x0)
    }, 7);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x10);
    CHECK_EQ(bus.load(RESULT1, WORD), 0x13);
    CHECK_EQ(bus.load(RESULT2, WORD), 0x11);
  });
}

static void test_csr_high_address() {
  // csr 0xBC0 has bit 11 set -> the sign-extended I-imm is negative; the
  // & 0xFFF in execute must recover the address (else: OOB csr[] index)
  guarded("csr high addr", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x07700293, // addi t0, x0, 0x77
      0xBC029073, // csrw 0xbc0, t0
      0xBC002373, // csrr t1, 0xbc0
      0x20602023, // sw   t1, 0x200(x0)
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x77);
  });
}

static void test_csrrw_rd_equals_rs1() {
  // csrrw t0, mscratch, t0 — old CSR and old t0 must swap cleanly
  guarded("csrrw alias", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x0AA00293, // addi  t0, x0, 0xAA
      0x34029073, // csrw  mscratch, t0        mscratch = 0xAA
      0x0BB00293, // addi  t0, x0, 0xBB
      0x340292F3, // csrrw t0, mscratch, t0    t0 = 0xAA, mscratch = 0xBB
      0x20502023, // sw    t0, 0x200(x0)
      0x34002373, // csrr  t1, mscratch
      0x20602223, // sw    t1, 0x204(x0)
    }, 7);
    CHECK_EQ(bus.load(RESULT0, WORD), 0xAA);
    CHECK_EQ(bus.load(RESULT1, WORD), 0xBB);
  });
}

// ------------------------------------------------------------- traps
//
// Layout of these programs: main flow at 0x0, nop padding, trap handler
// further up (address loaded into mtvec at the start). Results observed
// via stores, as always.

static void test_ecall_roundtrip() {
  // ecall -> handler reads mepc/mcause, steps mepc past the ecall, mret,
  // main flow continues after the ecall
  guarded("ecall roundtrip", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x04000293, // 0x00: addi t0, x0, 0x40     handler address
      0x30529073, // 0x04: csrw mtvec, t0
      0x00000073, // 0x08: ecall                 -> handler
      0x02A00313, // 0x0C: addi t1, x0, 42       (must run AFTER the handler)
      0x20602023, // 0x10: sw   t1, 0x200(x0)
      0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000013, // nop padding
      0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000013,
      0x00000013,
      0x341023F3, // 0x40: csrr t2, mepc
      0x20702223, // 0x44: sw   t2, 0x204(x0)    RESULT1 = mepc
      0x342023F3, // 0x48: csrr t2, mcause
      0x20702423, // 0x4C: sw   t2, 0x208(x0)    RESULT2 = mcause
      0x341023F3, // 0x50: csrr t2, mepc
      0x00438393, // 0x54: addi t2, t2, 4        step past the ecall
      0x34139073, // 0x58: csrw mepc, t2
      0x30200073, // 0x5C: mret                  -> 0x0C
    }, 13);
    CHECK_EQ(bus.load(RESULT0, WORD), 42);   // came back and continued
    CHECK_EQ(bus.load(RESULT1, WORD), 0x08); // mepc = address of the ecall itself
    CHECK_EQ(bus.load(RESULT2, WORD), 11);   // mcause = ecall from M-mode
  });
}

static void test_ebreak_cause_and_tval() {
  // ebreak -> mcause=3, mtval and mepc both hold the ebreak's address.
  // No mret here; the test ends inside the handler.
  guarded("ebreak cause/tval", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x02000293, // 0x00: addi t0, x0, 0x20     handler address
      0x30529073, // 0x04: csrw mtvec, t0
      0x00100073, // 0x08: ebreak                -> handler
      0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000013, // nop padding
      0x342023F3, // 0x20: csrr t2, mcause
      0x20702023, // 0x24: sw   t2, 0x200(x0)    RESULT0 = mcause
      0x343023F3, // 0x28: csrr t2, mtval
      0x20702223, // 0x2C: sw   t2, 0x204(x0)    RESULT1 = mtval
      0x341023F3, // 0x30: csrr t2, mepc
      0x20702423, // 0x34: sw   t2, 0x208(x0)    RESULT2 = mepc
    }, 9);
    CHECK_EQ(bus.load(RESULT0, WORD), 3);    // breakpoint
    CHECK_EQ(bus.load(RESULT1, WORD), 0x08); // mtval = ebreak address
    CHECK_EQ(bus.load(RESULT2, WORD), 0x08); // mepc  = ebreak address
  });
}

static void test_mstatus_push_pop() {
  // MIE on -> trap pushes it (MPIE<-MIE, MIE<-0, MPP<-3), mret pops it back
  guarded("mstatus push/pop", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x04000293, // 0x00: addi t0, x0, 0x40     handler address
      0x30529073, // 0x04: csrw mtvec, t0
      0x30046073, // 0x08: csrrsi x0, mstatus, 8 MIE <- 1
      0x00000073, // 0x0C: ecall                 -> handler
      0x30002373, // 0x10: csrr t1, mstatus      (after mret)
      0x20602423, // 0x14: sw   t1, 0x208(x0)    RESULT2 = restored mstatus
      0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000013, // nop padding
      0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000013,
      0x300023F3, // 0x40: csrr t2, mstatus      (inside handler)
      0x20702023, // 0x44: sw   t2, 0x200(x0)    RESULT0 = in-trap mstatus
      0x341023F3, // 0x48: csrr t2, mepc
      0x00438393, // 0x4C: addi t2, t2, 4
      0x34139073, // 0x50: csrw mepc, t2
      0x30200073, // 0x54: mret                  -> 0x10
    }, 12);
    // in handler: MIE(3)=0, MPIE(7)=1, MPP=3, plus hardwired FS=11|SD
    CHECK_EQ(bus.load(RESULT0, WORD), 0x80007880);
    // after mret: MIE restored to 1, MPIE=1, MPP stays M
    CHECK_EQ(bus.load(RESULT2, WORD), 0x80007888);
  });
}

static void test_reset_csr_values() {
  // capability CSRs after reset: misa says RV32I (MXL=1, I-bit), mstatus
  // says MPP=M — and misa must be write-protected by the WARL mask
  guarded("reset csr values", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30102373, // csrr  t1, misa
      0x20602023, // sw    t1, 0x200(x0)      RESULT0 = misa after reset
      0x30002E73, // csrr  t3, mstatus
      0x21C02423, // sw    t3, 0x208(x0)      RESULT2 = mstatus after reset
      0xFFF00293, // addi  t0, x0, -1
      0x30129073, // csrw  misa, t0           write attempt must bounce off
      0x301023F3, // csrr  t2, misa
      0x20702223, // sw    t2, 0x204(x0)      RESULT1 = misa after write
    }, 8);
    // update deliberately when the machine grows an extension!
    CHECK_EQ(bus.load(RESULT0, WORD), 0x40001129); // MXL=1 (RV32) | I|M|A|F|D
    CHECK_EQ(bus.load(RESULT1, WORD), 0x40001129); // unchanged: read-only
    // MPP=3 | FS=Dirty(11)<<13 | SD(31) — FS/SD hardwired since F/D landed
    CHECK_EQ(bus.load(RESULT2, WORD), 0x80007800);
  });
}

static void test_misaligned_fetch_trap() {
  // jalr to a target with bit 1 set: the JUMP traps (cause 0), mepc points
  // at the jalr, mtval holds the bad target — and rd must NOT be written
  guarded("misaligned fetch", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x04000293, // 0x00: addi t0, x0, 0x40     handler address
      0x30529073, // 0x04: csrw mtvec, t0
      0x07700313, // 0x08: addi t1, x0, 0x77     sentinel in the link register
      0x02000393, // 0x0C: addi t2, x0, 0x20
      0x00238367, // 0x10: jalr t1, t2, 2        -> 0x22: misaligned, must trap
      0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000013, // nop padding
      0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000013,
      0x00000013,
      0x34202E73, // 0x40: csrr t3, mcause
      0x21C02023, // 0x44: sw   t3, 0x200(x0)    RESULT0 = mcause
      0x34102E73, // 0x48: csrr t3, mepc
      0x21C02223, // 0x4C: sw   t3, 0x204(x0)    RESULT1 = mepc
      0x34302E73, // 0x50: csrr t3, mtval
      0x21C02423, // 0x54: sw   t3, 0x208(x0)    RESULT2 = mtval
      0x20602623, // 0x58: sw   t1, 0x20C(x0)    link reg: sentinel must survive
    }, 12);
    CHECK_EQ(bus.load(RESULT0, WORD), 0);    // cause 0 = misaligned fetch
    CHECK_EQ(bus.load(RESULT1, WORD), 0x10); // mepc = the jalr itself
    CHECK_EQ(bus.load(RESULT2, WORD), 0x22); // mtval = the bad target
    CHECK_EQ(bus.load(0x20C, WORD), 0x77);   // trapping instr wrote NO rd
  });
}

static void test_not_taken_branch_no_trap() {
  // the target of a branch that is NOT taken is never checked
  guarded("not-taken branch", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x00100293, // addi t0, x0, 1
      0x00028D63, // beq  t0? no: beq t0,x0,+26 -> misaligned target, NOT taken
      0x02A00313, // addi t1, x0, 42
      0x20602023, // sw   t1, 0x200(x0)
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 42); // fell through, no trap
  });
}

// ------------------------------------------------------------ atomics
//
// atomic target address is 0x300 (clear of the RESULT slots)

static void test_lr_sc_pair_and_consumption() {
  guarded("lr/sc", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30000293, // addi t0, x0, 0x300
      0x05500313, // addi t1, x0, 0x55
      0x0062A023, // sw   t1, 0(t0)         mem[0x300] = 0x55
      0x1002A32F, // lr.w t1, (t0)          t1 = 0x55, reservation on 0x300
      0x07700E13, // addi t3, x0, 0x77
      0x19C2A3AF, // sc.w t2, t3, (t0)      success: mem = 0x77, t2 = 0
      0x20702023, // sw   t2, 0x200(x0)     RESULT0 = 0 (sc succeeded)
      0x0002AE83, // lw   t4, 0(t0)
      0x21D02223, // sw   t4, 0x204(x0)     RESULT1 = 0x77 (store happened)
      0x19C2A3AF, // sc.w t2, t3, (t0)      reservation consumed -> must fail
      0x20702423, // sw   t2, 0x208(x0)     RESULT2 = nonzero
    }, 11);
    CHECK_EQ(bus.load(RESULT0, WORD), 0);
    CHECK_EQ(bus.load(RESULT1, WORD), 0x77);
    CHECK(bus.load(RESULT2, WORD) != 0); // second sc without reservation fails
  });
}

static void test_sc_wrong_address_fails() {
  guarded("sc wrong addr", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30000293, // addi t0, x0, 0x300
      0x30400313, // addi t1, x0, 0x304     a DIFFERENT address
      0x1002A3AF, // lr.w t2, (t0)          reservation on 0x300
      0x04200E13, // addi t3, x0, 0x42
      0x19C323AF, // sc.w t2, t3, (t1)      sc on 0x304 -> must fail
      0x20702023, // sw   t2, 0x200(x0)     RESULT0 = nonzero
      0x00032E83, // lw   t4, 0(t1)
      0x21D02223, // sw   t4, 0x204(x0)     RESULT1 = 0 (nothing was stored)
    }, 8);
    CHECK(bus.load(RESULT0, WORD) != 0); // sc paired with wrong address
    CHECK_EQ(bus.load(RESULT1, WORD), 0);
  });
}

static void test_amoswap_touches_only_rd_and_mem() {
  // rd <- old mem, mem <- rs2 — and the rs2 REGISTER stays untouched
  guarded("amoswap", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30000293, // addi t0, x0, 0x300
      0x0AA00313, // addi t1, x0, 0xAA
      0x0062A023, // sw   t1, 0(t0)         mem[0x300] = 0xAA
      0x0BB00313, // addi t1, x0, 0xBB      t1 (rs2) = 0xBB
      0x0862A3AF, // amoswap.w t2, t1, (t0) t2 = 0xAA, mem = 0xBB
      0x20702023, // sw   t2, 0x200(x0)     RESULT0 = 0xAA (old value into rd)
      0x20602223, // sw   t1, 0x204(x0)     RESULT1 = 0xBB (rs2 reg untouched!)
      0x0002AE83, // lw   t4, 0(t0)
      0x21D02423, // sw   t4, 0x208(x0)     RESULT2 = 0xBB (rs2 into memory)
    }, 9);
    CHECK_EQ(bus.load(RESULT0, WORD), 0xAA);
    CHECK_EQ(bus.load(RESULT1, WORD), 0xBB);
    CHECK_EQ(bus.load(RESULT2, WORD), 0xBB);
  });
}

static void test_fence_variants_are_nops() {
  // no caches, no other harts: all three fences complete as nops and
  // execution continues
  guarded("fence nops", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x0FF0000F, // fence iorw, iorw
      0x0000100F, // fence.i  (Zifencei)
      0x8330000F, // fence.tso
      0x02A00313, // addi t1, x0, 42
      0x20602023, // sw   t1, 0x200(x0)
    }, 5);
    CHECK_EQ(bus.load(RESULT0, WORD), 42);
  });
}

static void test_wfi_is_nop() {
  // no interrupts exist yet -> spec allows WFI to complete as a nop
  guarded("wfi nop", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x10500073, // wfi
      0x02A00313, // addi t1, x0, 42
      0x20602023, // sw   t1, 0x200(x0)
    }, 3);
    CHECK_EQ(bus.load(RESULT0, WORD), 42);
  });
}

// ------------------------------------------------- FP loads/stores (F/D)
//
// No FP arithmetic yet — these pin down the memory path and the register
// file semantics: NaN-boxing on FLW, little-endian word order on FLD/FSD,
// and FSW storing exactly the low 4 bytes. FP data staged at 0x300 via
// integer stores; fsd is the only way to observe all 64 register bits.

static void test_flw_nan_boxing() {
  // FLW must box the 32-bit value: upper half of the f-register all-ones
  guarded("flw nan-box", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x11100293, // addi t0, x0, 0x111
      0x30502023, // sw   t0, 0x300(x0)
      0x30002087, // flw  ft1, 0x300(x0)    ft1 = 0xFFFFFFFF_00000111
      0x20103027, // fsd  ft1, 0x200(x0)    dump all 64 bits
    }, 4);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x111);      // payload
    CHECK_EQ(bus.load(RESULT1, WORD), 0xFFFFFFFF); // the NaN box
  });
}

static void test_fld_fsd_word_order() {
  // fld from two known words, then observe the LOW half alone via fsw
  // (catches a swapped load even if fsd swaps identically), then the full
  // register via fsd (pins the store direction)
  guarded("fld/fsd order", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x11100293, // addi t0, x0, 0x111
      0x22200313, // addi t1, x0, 0x222
      0x30502023, // sw   t0, 0x300(x0)     low  word
      0x30602223, // sw   t1, 0x304(x0)     high word
      0x30003087, // fld  ft1, 0x300(x0)    ft1 = 0x00000222_00000111
      0x20102427, // fsw  ft1, 0x208(x0)    RESULT2 = low half only
      0x20103027, // fsd  ft1, 0x200(x0)    RESULT0/1 = full register
    }, 7);
    CHECK_EQ(bus.load(RESULT2, WORD), 0x111); // load kept addr as low word
    CHECK_EQ(bus.load(RESULT0, WORD), 0x111); // store put low word at addr
    CHECK_EQ(bus.load(RESULT1, WORD), 0x222); // ... and high at addr+4
  });
}

static void test_fsw_writes_four_bytes() {
  // fsw of a register holding a 64-bit value must write exactly 4 bytes —
  // a sentinel in the adjacent word must survive
  guarded("fsw width", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x11100293, // addi t0, x0, 0x111
      0x22200313, // addi t1, x0, 0x222
      0x30502023, // sw   t0, 0x300(x0)
      0x30602223, // sw   t1, 0x304(x0)
      0xFFF00393, // addi t2, x0, -1
      0x20702223, // sw   t2, 0x204(x0)     sentinel next to RESULT0
      0x30003087, // fld  ft1, 0x300(x0)    full 64-bit value
      0x20102027, // fsw  ft1, 0x200(x0)    low word only
    }, 8);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x111);      // low half stored
    CHECK_EQ(bus.load(RESULT1, WORD), 0xFFFFFFFF); // sentinel untouched
  });
}

// ------------------------------------------------- fused multiply-add (F/D)
//
// FP constants are staged in RAM via lui/sw and loaded with flw/fld; results
// observed as raw bit patterns via fsw/fsd. The four fused variants are
// checked against values that make every sign mistake visible:
//   a=2, b=3, c=5:  fmadd=11  fmsub=1  fnmsub=-1  fnmadd=-11

static void test_fma_s_four_variants() {
  guarded("fma.s variants", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x400002B7, // lui  t0, 0x40000       2.0f
      0x30502023, // sw   t0, 0x300(x0)
      0x404002B7, // lui  t0, 0x40400       3.0f
      0x30502223, // sw   t0, 0x304(x0)
      0x40A002B7, // lui  t0, 0x40A00       5.0f
      0x30502423, // sw   t0, 0x308(x0)
      0x30002087, // flw  ft1, 0x300(x0)
      0x30402107, // flw  ft2, 0x304(x0)
      0x30802187, // flw  ft3, 0x308(x0)
      0x1820F243, // fmadd.s  ft4, ft1, ft2, ft3     2*3+5 = 11
      0x20402027, // fsw  ft4, 0x200(x0)
      0x1820F247, // fmsub.s  ft4, ft1, ft2, ft3     2*3-5 = 1
      0x20402227, // fsw  ft4, 0x204(x0)
      0x1820F24B, // fnmsub.s ft4, ft1, ft2, ft3    -2*3+5 = -1
      0x20402427, // fsw  ft4, 0x208(x0)
      0x1820F24F, // fnmadd.s ft4, ft1, ft2, ft3    -2*3-5 = -11
      0x20402627, // fsw  ft4, 0x20C(x0)
    }, 17);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x41300000); //  11.0f
    CHECK_EQ(bus.load(RESULT1, WORD), 0x3F800000); //   1.0f
    CHECK_EQ(bus.load(RESULT2, WORD), 0xBF800000); //  -1.0f
    CHECK_EQ(bus.load(0x20C, WORD),   0xC1300000); // -11.0f
  });
}

static void test_fma_single_rounding() {
  // a = 1+2^-12: the true square 1 + 2^-11 + 2^-24 needs 25 significand
  // bits, so a separate multiply rounds the 2^-24 away and (a*a)+c gives
  // exactly 0. A REAL fused op keeps it and yields 2^-24. Pins std::fma
  // against the (a*b)+c shortcut.
  guarded("fma fusion", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x3F8012B7, // lui  t0, 0x3F801
      0x80028293, // addi t0, t0, -2048     t0 = 0x3F800800 = 1+2^-12
      0x30502023, // sw   t0, 0x300(x0)
      0xBF8012B7, // lui  t0, 0xBF801       -(1+2^-11)
      0x30502223, // sw   t0, 0x304(x0)
      0x30002087, // flw  ft1, 0x300(x0)
      0x30402187, // flw  ft3, 0x304(x0)
      0x1810F243, // fmadd.s ft4, ft1, ft1, ft3
      0x20402027, // fsw  ft4, 0x200(x0)
    }, 9);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x33800000); // 2^-24; 0 = not fused
  });
}

static void test_fma_sign_of_zero() {
  // exact cancellation must give +0. Formulating fnmsub as -(fma(a,b,-c))
  // instead of fma(-a,b,c) shows up here as -0 (0x80000000).
  guarded("fma zero sign", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x3F8002B7, // lui  t0, 0x3F800       1.0f
      0x30502023, // sw   t0, 0x300(x0)
      0x30002087, // flw  ft1, 0x300(x0)
      0x0810F24B, // fnmsub.s ft4, ft1, ft1, ft1    -(1*1)+1 = +0
      0x20402027, // fsw  ft4, 0x200(x0)
      0x0810F247, // fmsub.s  ft4, ft1, ft1, ft1      1*1-1 = +0
      0x20402227, // fsw  ft4, 0x204(x0)
    }, 7);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x00000000); // +0, NOT 0x80000000
    CHECK_EQ(bus.load(RESULT1, WORD), 0x00000000);
  });
}

static void test_fma_d_four_variants() {
  guarded("fma.d variants", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30002023, // sw   x0, 0x300(x0)     low(2.0)
      0x400002B7, // lui  t0, 0x40000
      0x30502223, // sw   t0, 0x304(x0)     high(2.0)
      0x30002423, // sw   x0, 0x308(x0)     low(3.0)
      0x400802B7, // lui  t0, 0x40080
      0x30502623, // sw   t0, 0x30C(x0)     high(3.0)
      0x30002823, // sw   x0, 0x310(x0)     low(5.0)
      0x401402B7, // lui  t0, 0x40140
      0x30502A23, // sw   t0, 0x314(x0)     high(5.0)
      0x30003087, // fld  ft1, 0x300(x0)
      0x30803107, // fld  ft2, 0x308(x0)
      0x31003187, // fld  ft3, 0x310(x0)
      0x1A20F243, // fmadd.d  ft4, ft1, ft2, ft3
      0x20403027, // fsd  ft4, 0x200(x0)
      0x1A20F247, // fmsub.d  ft4, ft1, ft2, ft3
      0x20403427, // fsd  ft4, 0x208(x0)
      0x1A20F24B, // fnmsub.d ft4, ft1, ft2, ft3
      0x20403827, // fsd  ft4, 0x210(x0)
      0x1A20F24F, // fnmadd.d ft4, ft1, ft2, ft3
      0x20403C27, // fsd  ft4, 0x218(x0)
    }, 20);
    CHECK_EQ(bus.load(0x200, WORD), 0);          //  11.0 low
    CHECK_EQ(bus.load(0x204, WORD), 0x40260000); //  11.0 high
    CHECK_EQ(bus.load(0x208, WORD), 0);          //   1.0 low
    CHECK_EQ(bus.load(0x20C, WORD), 0x3FF00000); //   1.0 high
    CHECK_EQ(bus.load(0x210, WORD), 0);          //  -1.0 low
    CHECK_EQ(bus.load(0x214, WORD), 0xBFF00000); //  -1.0 high
    CHECK_EQ(bus.load(0x218, WORD), 0);          // -11.0 low
    CHECK_EQ(bus.load(0x21C, WORD), 0xC0260000); // -11.0 high
  });
}

static void test_fma_s_consumes_nan_box() {
  // ft1 holds a plain double (upper bits NOT all-ones) — a .s op reading it
  // must see canonical NaN, so NaN*1+1 = NaN, not 2*1+1 = 3.
  // Exact-canonical check (0x7FC00000): holds because the boxed-read rule
  // injects the canonical pattern and hosts propagate that payload; loosen
  // to an is-NaN check only if a host is ever found violating it.
  guarded("fma.s box check", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30002023, // sw   x0, 0x300(x0)
      0x400002B7, // lui  t0, 0x40000
      0x30502223, // sw   t0, 0x304(x0)     2.0 (double) at 0x300
      0x30003087, // fld  ft1, 0x300(x0)    ft1 = 2.0, upper bits 0x40000000
      0x3F8002B7, // lui  t0, 0x3F800
      0x30502423, // sw   t0, 0x308(x0)
      0x30802107, // flw  ft2, 0x308(x0)    ft2 = 1.0f, properly boxed
      0x1020F243, // fmadd.s ft4, ft1, ft2, ft2
      0x20402027, // fsw  ft4, 0x200(x0)
    }, 9);
    CHECK_EQ(bus.load(RESULT0, WORD), 0x7FC00000); // canonical NaN, not 3.0f
  });
}

// ------------------------------------------------------ FP_ALU single (F)
//
// Golden words from riscv64-elf-as as usual. Constants staged at 0x300+ via
// integer stores, results observed as raw bit patterns via fsw (f-results)
// or sw (x-results).

static void test_fp_s_arithmetic() {
  guarded("f arith", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x400002B7, 0x30502023, // 2.0f -> 0x300
      0x404002B7, 0x30502223, // 3.0f -> 0x304
      0x30002087, 0x30402107, // flw ft1, ft2
      0x0020F1D3, 0x20302027, // fadd.s  ft3,ft1,ft2 ; fsw 0x200
      0x0820F1D3, 0x20302227, // fsub.s              ; fsw 0x204
      0x1020F1D3, 0x20302427, // fmul.s              ; fsw 0x208
      0x181171D3, 0x20302627, // fdiv.s  ft3,ft2,ft1 ; fsw 0x20C
      0x408002B7, 0x30502023, // 4.0f -> 0x300
      0x30002087,             // flw ft1
      0x5800F1D3, 0x20302827, // fsqrt.s ft3,ft1     ; fsw 0x210
      0x30002023, 0x30002107, // 0.0f -> 0x300; flw ft2
      0x3F8002B7, 0x30502223, // 1.0f -> 0x304
      0x30402087,             // flw ft1
      0x1820F1D3, 0x20302A27, // fdiv.s ft3,ft1,ft2 (1/0) ; fsw 0x214
    }, 26);
    CHECK_EQ(bus.load(0x200, WORD), 0x40A00000); // 5.0f
    CHECK_EQ(bus.load(0x204, WORD), 0xBF800000); // -1.0f
    CHECK_EQ(bus.load(0x208, WORD), 0x40C00000); // 6.0f
    CHECK_EQ(bus.load(0x20C, WORD), 0x3FC00000); // 1.5f
    CHECK_EQ(bus.load(0x210, WORD), 0x40000000); // sqrt(4) = 2.0f
    CHECK_EQ(bus.load(0x214, WORD), 0x7F800000); // 1/0 = +inf (IEEE, no trap)
  });
}

static void test_fp_s_min_max() {
  // the riscv-tests classics: NaN handling and the sign of zero
  guarded("fmin/fmax", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x3F8002B7, 0x30502023, // 1.0f -> 0x300
      0x400002B7, 0x30502223, // 2.0f -> 0x304
      0x30002087, 0x30402107, // flw ft1, ft2
      0x282081D3, 0x20302027, // fmin.s ft3,ft1,ft2 ; fsw 0x200
      0x282091D3, 0x20302227, // fmax.s             ; fsw 0x204
      0x800002B7, 0x30502023, // -0.0f -> 0x300
      0x30002223,             // +0.0f -> 0x304
      0x30002087, 0x30402107, // ft1 = -0, ft2 = +0
      0x281101D3, 0x20302427, // fmin.s ft3,ft2,ft1 (+0,-0) ; fsw 0x208
      0x282091D3, 0x20302627, // fmax.s ft3,ft1,ft2 (-0,+0) ; fsw 0x20C
      0x281091D3, 0x20302827, // fmax.s ft3,ft1,ft1 (-0,-0) ; fsw 0x210
      0x282101D3, 0x20302A27, // fmin.s ft3,ft2,ft2 (+0,+0) ; fsw 0x214
      0x7FC002B7, 0x30502023, // qNaN -> 0x300
      0x30002087,             // ft1 = NaN
      0x3F8002B7, 0x30502223, // 1.0f -> 0x304
      0x30402107,             // ft2 = 1.0
      0x282081D3, 0x20302C27, // fmin.s ft3,ft1,ft2 (NaN,1) ; fsw 0x218
      0x281091D3, 0x20302E27, // fmax.s ft3,ft1,ft1 (NaN,NaN) ; fsw 0x21C
    }, 33);
    CHECK_EQ(bus.load(0x200, WORD), 0x3F800000); // min(1,2) = 1
    CHECK_EQ(bus.load(0x204, WORD), 0x40000000); // max(1,2) = 2
    CHECK_EQ(bus.load(0x208, WORD), 0x80000000); // min(+0,-0) = -0
    CHECK_EQ(bus.load(0x20C, WORD), 0x00000000); // max(-0,+0) = +0
    CHECK_EQ(bus.load(0x210, WORD), 0x80000000); // max(-0,-0) = -0
    CHECK_EQ(bus.load(0x214, WORD), 0x00000000); // min(+0,+0) = +0
    CHECK_EQ(bus.load(0x218, WORD), 0x3F800000); // one NaN -> the other operand
    CHECK_EQ(bus.load(0x21C, WORD), 0x7FC00000); // both NaN -> canonical
  });
}

static void test_fp_s_sgnj() {
  guarded("fsgnj", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x400002B7, 0x30502023, // 2.0f -> 0x300
      0xC04002B7, 0x30502223, // -3.0f -> 0x304
      0x30002087, 0x30402107, // flw ft1, ft2
      0x202081D3, 0x20302027, // fsgnj.s  ft3,ft1,ft2 ; fsw 0x200
      0x202091D3, 0x20302227, // fsgnjn.s ft3,ft1,ft2 ; fsw 0x204
      0x202121D3, 0x20302427, // fsgnjx.s ft3,ft2,ft2 = fabs.s ; fsw 0x208
      0x201091D3, 0x20302627, // fsgnjn.s ft3,ft1,ft1 = fneg.s ; fsw 0x20C
      0x7F8002B7, 0x00128293, // t0 = 0x7F800001 (sNaN with payload 1)
      0x30502023, 0x30002087, // -> 0x300; flw ft1
      0x201081D3, 0x20302827, // fsgnj.s ft3,ft1,ft1 = fmv.s ; fsw 0x210
    }, 20);
    CHECK_EQ(bus.load(0x200, WORD), 0xC0000000); // 2.0 with -3.0's sign
    CHECK_EQ(bus.load(0x204, WORD), 0x40000000); // 2.0 with inverted sign
    CHECK_EQ(bus.load(0x208, WORD), 0x40400000); // fabs(-3) = 3.0
    CHECK_EQ(bus.load(0x20C, WORD), 0xC0000000); // fneg(2) = -2.0
    // sgnj is a bit operation: the sNaN payload must survive verbatim,
    // NOT get canonicalized to 0x7FC00000
    CHECK_EQ(bus.load(0x210, WORD), 0x7F800001);
  });
}

static void test_fp_s_compare() {
  guarded("feq/flt/fle", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x3F8002B7, 0x30502023, // 1.0f -> 0x300
      0x400002B7, 0x30502223, // 2.0f -> 0x304
      0x7FC002B7, 0x30502423, // qNaN -> 0x308
      0x30002087, 0x30402107, 0x30802187, // flw ft1, ft2, ft3
      0xA0209353, 0x20602023, // flt.s t1,ft1,ft2 (1<2)   ; sw 0x200
      0xA0108353, 0x20602223, // fle.s t1,ft1,ft1 (1<=1)  ; sw 0x204
      0xA020A353, 0x20602423, // feq.s t1,ft1,ft2 (1==2)  ; sw 0x208
      0xA010A353, 0x20602623, // feq.s t1,ft1,ft1 (1==1)  ; sw 0x20C
      0xA031A353, 0x20602823, // feq.s t1,ft3,ft3 (NaN)   ; sw 0x210
      0xA0119353, 0x20602A23, // flt.s t1,ft3,ft1 (NaN<1) ; sw 0x214
      0x800002B7, 0x30502023, // -0.0f -> 0x300
      0x30002087,             // ft1 = -0
      0x30002223, 0x30402107, // +0.0f -> 0x304; ft2 = +0
      0xA020A353, 0x20602C23, // feq.s t1,ft1,ft2 (-0==+0); sw 0x218
    }, 28);
    CHECK_EQ(bus.load(0x200, WORD), 1); // 1 < 2
    CHECK_EQ(bus.load(0x204, WORD), 1); // 1 <= 1
    CHECK_EQ(bus.load(0x208, WORD), 0); // 1 == 2
    CHECK_EQ(bus.load(0x20C, WORD), 1); // 1 == 1
    CHECK_EQ(bus.load(0x210, WORD), 0); // NaN == NaN is false
    CHECK_EQ(bus.load(0x214, WORD), 0); // NaN < x is false
    CHECK_EQ(bus.load(0x218, WORD), 1); // -0 == +0 per IEEE
  });
}

static void test_fp_s_fclass() {
  // one probe per class, results in slots 0x200 + 4*i, expected value 1<<i
  guarded("fclass", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0xFF8002B7, 0x30502023, 0x30002087, 0xE0009353, 0x20602023, // -inf
      0xBF8002B7, 0x30502023, 0x30002087, 0xE0009353, 0x20602223, // -1.0
      0x800002B7, 0x00128293, 0x30502023, 0x30002087, 0xE0009353,
      0x20602423,                                                 // -subnormal
      0x800002B7, 0x30502023, 0x30002087, 0xE0009353, 0x20602623, // -0
      0x30002023, 0x30002087, 0xE0009353, 0x20602823,             // +0
      0x00100293, 0x30502023, 0x30002087, 0xE0009353, 0x20602A23, // +subnormal
      0x3F8002B7, 0x30502023, 0x30002087, 0xE0009353, 0x20602C23, // +1.0
      0x7F8002B7, 0x30502023, 0x30002087, 0xE0009353, 0x20602E23, // +inf
      0x7F8002B7, 0x00128293, 0x30502023, 0x30002087, 0xE0009353,
      0x22602023,                                                 // sNaN
      0x7FC002B7, 0x30502023, 0x30002087, 0xE0009353, 0x22602223, // qNaN
    }, 51);
    for (int i = 0; i < 10; i++)
      CHECK_EQ(bus.load(0x200 + 4 * i, WORD), 1u << i);
  });
}

static void test_fp_s_cvt_to_int() {
  // the saturation table of the F chapter, plus in-range values chosen so
  // truncation and round-nearest-even agree (rm handling comes later)
  guarded("fcvt.w/wu.s", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x401002B7, 0x30502023, 0x30002087, 0xC000F353, 0x20602023, //  2.25
      0xC01002B7, 0x30502023, 0x30002087, 0xC000F353, 0x20602223, // -2.25
      0x7F8002B7, 0x30502023, 0x30002087, 0xC000F353, 0x20602423, // w(+inf)
      0xFF8002B7, 0x30502023, 0x30002087, 0xC000F353, 0x20602623, // w(-inf)
      0x7FC002B7, 0x30502023, 0x30002087, 0xC000F353, 0x20602823, // w(NaN)
      0xC010F353, 0x20602A23,                                     // wu(NaN)
      0xBF8002B7, 0x30502023, 0x30002087, 0xC010F353, 0x20602C23, // wu(-1.0)
      0x4F0002B7, 0x30502023, 0x30002087, 0xC000F353, 0x20602E23, // w(2^31)
      0xC010F353, 0x22602023,                                     // wu(2^31)
      0x4F8002B7, 0x30502023, 0x30002087, 0xC010F353, 0x22602223, // wu(2^32)
    }, 44);
    CHECK_EQ(bus.load(0x200, WORD), 2);          // trunc(2.25)
    CHECK_EQ(bus.load(0x204, WORD), 0xFFFFFFFE); // trunc(-2.25) = -2
    CHECK_EQ(bus.load(0x208, WORD), 0x7FFFFFFF); // +inf saturates to INT_MAX
    CHECK_EQ(bus.load(0x20C, WORD), 0x80000000); // -inf saturates to INT_MIN
    CHECK_EQ(bus.load(0x210, WORD), 0x7FFFFFFF); // NaN -> INT_MAX (positive!)
    CHECK_EQ(bus.load(0x214, WORD), 0xFFFFFFFF); // NaN -> UINT_MAX
    CHECK_EQ(bus.load(0x218, WORD), 0);          // wu(-1.0) saturates to 0
    CHECK_EQ(bus.load(0x21C, WORD), 0x7FFFFFFF); // w(2^31) saturates
    CHECK_EQ(bus.load(0x220, WORD), 0x80000000); // wu(2^31) is VALID
    CHECK_EQ(bus.load(0x224, WORD), 0xFFFFFFFF); // wu(2^32) saturates
  });
}

static void test_fp_s_cvt_from_int_and_fmv() {
  guarded("fcvt.s.w/fmv", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0xFFF00293,             // t0 = 0xFFFFFFFF
      0xD002F0D3, 0x20102027, // fcvt.s.w  ft1,t0 (-1)  ; fsw 0x200
      0xD012F0D3, 0x20102227, // fcvt.s.wu ft1,t0 (max) ; fsw 0x204
      0xC00002B7,             // t0 = 0xC0000000 (bits of -2.0f)
      0xF00280D3, 0x20102427, // fmv.w.x ft1,t0 ; fsw 0x208
      0xE0008353, 0x20602623, // fmv.x.w t1,ft1 ; sw 0x20C
      0x30002023, 0x400002B7, // double 2.0 -> 0x300/0x304
      0x30502223, 0x30003087, // ... ; fld ft1 (NOT boxed)
      0xE0008353, 0x20602823, // fmv.x.w t1,ft1 ; sw 0x210
    }, 16);
    CHECK_EQ(bus.load(0x200, WORD), 0xBF800000); // (float)(i32)-1 = -1.0f
    // 4294967295 rounds up to 2^32 under round-nearest-even
    CHECK_EQ(bus.load(0x204, WORD), 0x4F800000);
    CHECK_EQ(bus.load(0x208, WORD), 0xC0000000); // fmv.w.x: raw bits in
    CHECK_EQ(bus.load(0x20C, WORD), 0xC0000000); // fmv.x.w: raw bits out
    // fmv.x.w is a TRANSFER op: no box check, raw low word of the double
    // (2.0 has low word 0) — NOT canonical NaN
    CHECK_EQ(bus.load(0x210, WORD), 0x00000000);
  });
}

// ------------------------------------------------------ FP_ALU double (D)
//
// Mirror of the single-precision net with 64-bit constants (staged as two
// words, low at 0x300/0x308, high at 0x304/0x30C) and results read back as
// word pairs from fsd. Plus D-specific probes: fcvt.d.w exactness at 2^24+1
// (catches a float round-trip) and 32-bit saturation from double inputs.

static void test_fp_d_arithmetic() {
  guarded("d arith", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30002023, 0x400002B7, 0x30502223, // 2.0 -> 0x300
      0x30002423, 0x400802B7, 0x30502623, // 3.0 -> 0x308
      0x30003087, 0x30803107,             // fld ft1, ft2
      0x0220F1D3, 0x20303027,             // fadd.d ; fsd 0x200
      0x0A20F1D3, 0x20303427,             // fsub.d ; fsd 0x208
      0x1220F1D3, 0x20303827,             // fmul.d ; fsd 0x210
      0x1A1171D3, 0x20303C27,             // fdiv.d ft3,ft2,ft1 ; fsd 0x218
      0x401002B7, 0x30502223,             // 4.0 -> 0x300
      0x30003087,                         // fld ft1
      0x5A00F1D3, 0x22303027,             // fsqrt.d ; fsd 0x220
      0x30002423, 0x30002623,             // 0.0 -> 0x308
      0x30803107,                         // fld ft2
      0x3FF002B7, 0x30502223,             // 1.0 -> 0x300
      0x30003087,                         // fld ft1
      0x1A20F1D3, 0x22303427,             // fdiv.d (1/0) ; fsd 0x228
    }, 29);
    CHECK_EQ(bus.load(0x200, WORD), 0);          // 5.0
    CHECK_EQ(bus.load(0x204, WORD), 0x40140000);
    CHECK_EQ(bus.load(0x208, WORD), 0);          // -1.0
    CHECK_EQ(bus.load(0x20C, WORD), 0xBFF00000);
    CHECK_EQ(bus.load(0x210, WORD), 0);          // 6.0
    CHECK_EQ(bus.load(0x214, WORD), 0x40180000);
    CHECK_EQ(bus.load(0x218, WORD), 0);          // 1.5
    CHECK_EQ(bus.load(0x21C, WORD), 0x3FF80000);
    CHECK_EQ(bus.load(0x220, WORD), 0);          // sqrt(4) = 2.0
    CHECK_EQ(bus.load(0x224, WORD), 0x40000000);
    CHECK_EQ(bus.load(0x228, WORD), 0);          // 1/0 = +inf
    CHECK_EQ(bus.load(0x22C, WORD), 0x7FF00000);
  });
}

static void test_fp_d_min_max() {
  guarded("fmin/fmax.d", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30002023, 0x3FF002B7, 0x30502223, // 1.0 -> 0x300
      0x30002423, 0x400002B7, 0x30502623, // 2.0 -> 0x308
      0x30003087, 0x30803107,             // fld ft1, ft2
      0x2A2081D3, 0x20303027,             // fmin.d ; fsd 0x200
      0x2A2091D3, 0x20303427,             // fmax.d ; fsd 0x208
      0x800002B7, 0x30502223,             // -0.0 -> 0x300
      0x30002623,                         // +0.0 -> 0x308
      0x30003087, 0x30803107,             // ft1 = -0, ft2 = +0
      0x2A1101D3, 0x20303827,             // fmin.d(+0,-0) ; fsd 0x210
      0x2A2091D3, 0x20303C27,             // fmax.d(-0,+0) ; fsd 0x218
      0x2A1091D3, 0x22303027,             // fmax.d(-0,-0) ; fsd 0x220
      0x2A2101D3, 0x22303427,             // fmin.d(+0,+0) ; fsd 0x228
      0x7FF802B7, 0x30502223,             // qNaN -> 0x300
      0x3FF002B7, 0x30502623,             // 1.0 -> 0x308
      0x30003087, 0x30803107,             // fld ft1, ft2
      0x2A2081D3, 0x22303827,             // fmin.d(NaN,1) ; fsd 0x230
      0x2A1091D3, 0x22303C27,             // fmax.d(NaN,NaN) ; fsd 0x238
    }, 35);
    CHECK_EQ(bus.load(0x200, WORD), 0);          // min(1,2) = 1.0
    CHECK_EQ(bus.load(0x204, WORD), 0x3FF00000);
    CHECK_EQ(bus.load(0x208, WORD), 0);          // max(1,2) = 2.0
    CHECK_EQ(bus.load(0x20C, WORD), 0x40000000);
    CHECK_EQ(bus.load(0x210, WORD), 0);          // min(+0,-0) = -0
    CHECK_EQ(bus.load(0x214, WORD), 0x80000000);
    CHECK_EQ(bus.load(0x218, WORD), 0);          // max(-0,+0) = +0
    CHECK_EQ(bus.load(0x21C, WORD), 0x00000000);
    CHECK_EQ(bus.load(0x220, WORD), 0);          // max(-0,-0) = -0
    CHECK_EQ(bus.load(0x224, WORD), 0x80000000);
    CHECK_EQ(bus.load(0x228, WORD), 0);          // min(+0,+0) = +0
    CHECK_EQ(bus.load(0x22C, WORD), 0x00000000);
    CHECK_EQ(bus.load(0x230, WORD), 0);          // one NaN -> other = 1.0
    CHECK_EQ(bus.load(0x234, WORD), 0x3FF00000);
    CHECK_EQ(bus.load(0x238, WORD), 0);          // both NaN -> canonical
    CHECK_EQ(bus.load(0x23C, WORD), 0x7FF80000);
  });
}

static void test_fp_d_sgnj() {
  guarded("fsgnj.d", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30002023, 0x400002B7, 0x30502223, // 2.0 -> 0x300
      0x30002423, 0xC00802B7, 0x30502623, // -3.0 -> 0x308
      0x30003087, 0x30803107,             // fld ft1, ft2
      0x222081D3, 0x20303027,             // fsgnj.d  ; fsd 0x200
      0x222091D3, 0x20303427,             // fsgnjn.d ; fsd 0x208
      0x222121D3, 0x20303827,             // fsgnjx.d ft3,ft2,ft2 = fabs ; 0x210
      0x221091D3, 0x20303C27,             // fsgnjn.d ft3,ft1,ft1 = fneg ; 0x218
      0x00100293, 0x30502023,             // low word 1 -> 0x300
      0x7FF002B7, 0x30502223,             // sNaN 0x7FF00000_00000001
      0x30003087,                         // fld ft1
      0x221081D3, 0x22303027,             // fsgnj.d = fmv.d ; fsd 0x220
    }, 23);
    CHECK_EQ(bus.load(0x200, WORD), 0);          // -2.0
    CHECK_EQ(bus.load(0x204, WORD), 0xC0000000);
    CHECK_EQ(bus.load(0x208, WORD), 0);          // +2.0
    CHECK_EQ(bus.load(0x20C, WORD), 0x40000000);
    CHECK_EQ(bus.load(0x210, WORD), 0);          // fabs(-3) = 3.0
    CHECK_EQ(bus.load(0x214, WORD), 0x40080000);
    CHECK_EQ(bus.load(0x218, WORD), 0);          // fneg(2) = -2.0
    CHECK_EQ(bus.load(0x21C, WORD), 0xC0000000);
    CHECK_EQ(bus.load(0x220, WORD), 1);          // sNaN payload survives
    CHECK_EQ(bus.load(0x224, WORD), 0x7FF00000); // verbatim
  });
}

static void test_fp_d_compare() {
  guarded("feq/flt/fle.d", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30002023, 0x3FF002B7, 0x30502223, // 1.0 -> 0x300
      0x30002423, 0x400002B7, 0x30502623, // 2.0 -> 0x308
      0x30002823, 0x7FF802B7, 0x30502A23, // qNaN -> 0x310
      0x30003087, 0x30803107, 0x31003187, // fld ft1, ft2, ft3
      0xA2209353, 0x20602023,             // flt.d 1<2  ; sw 0x200
      0xA2108353, 0x20602223,             // fle.d 1<=1 ; sw 0x204
      0xA220A353, 0x20602423,             // feq.d 1==2 ; sw 0x208
      0xA210A353, 0x20602623,             // feq.d 1==1 ; sw 0x20C
      0xA231A353, 0x20602823,             // feq.d NaN  ; sw 0x210
      0xA2119353, 0x20602A23,             // flt.d NaN<1; sw 0x214
      0x800002B7, 0x30502223,             // -0.0 -> 0x300
      0x30003087,                         // ft1 = -0
      0x30002623, 0x30803107,             // +0.0 -> 0x308; ft2 = +0
      0xA220A353, 0x20602C23,             // feq.d -0==+0 ; sw 0x218
    }, 31);
    CHECK_EQ(bus.load(0x200, WORD), 1);
    CHECK_EQ(bus.load(0x204, WORD), 1);
    CHECK_EQ(bus.load(0x208, WORD), 0);
    CHECK_EQ(bus.load(0x20C, WORD), 1);
    CHECK_EQ(bus.load(0x210, WORD), 0); // NaN == NaN false
    CHECK_EQ(bus.load(0x214, WORD), 0); // NaN < x false
    CHECK_EQ(bus.load(0x218, WORD), 1); // -0 == +0
  });
}

static void test_fp_d_fclass() {
  guarded("fclass.d", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30002023, 0xFFF002B7, 0x30502223, 0x30003087, 0xE2009353,
      0x20602023,                                              // -inf
      0xBFF002B7, 0x30502223, 0x30003087, 0xE2009353,
      0x20602223,                                              // -1.0
      0x00100293, 0x30502023, 0x800002B7, 0x30502223,
      0x30003087, 0xE2009353, 0x20602423,                      // -subnormal
      0x30002023, 0x800002B7, 0x30502223, 0x30003087,
      0xE2009353, 0x20602623,                                  // -0
      0x30002223, 0x30003087, 0xE2009353, 0x20602823,          // +0
      0x00100293, 0x30502023, 0x30003087, 0xE2009353,
      0x20602A23,                                              // +subnormal
      0x30002023, 0x3FF002B7, 0x30502223, 0x30003087,
      0xE2009353, 0x20602C23,                                  // +1.0
      0x7FF002B7, 0x30502223, 0x30003087, 0xE2009353,
      0x20602E23,                                              // +inf
      0x00100293, 0x30502023, 0x7FF002B7, 0x30502223,
      0x30003087, 0xE2009353, 0x22602023,                      // sNaN
      0x30002023, 0x7FF802B7, 0x30502223, 0x30003087,
      0xE2009353, 0x22602223,                                  // qNaN
    }, 57);
    for (int i = 0; i < 10; i++)
      CHECK_EQ(bus.load(0x200 + 4 * i, WORD), 1u << i);
  });
}

static void test_fp_d_cvt_to_int() {
  // 32-bit saturation from DOUBLE inputs — would catch 64-bit bounds
  guarded("fcvt.w/wu.d", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x30002023, 0x400202B7, 0x30502223, 0x30003087, 0xC200F353,
      0x20602023,                                              //  2.25
      0xC00202B7, 0x30502223, 0x30003087, 0xC200F353,
      0x20602223,                                              // -2.25
      0x7FF002B7, 0x30502223, 0x30003087, 0xC200F353,
      0x20602423,                                              // w(+inf)
      0xFFF002B7, 0x30502223, 0x30003087, 0xC200F353,
      0x20602623,                                              // w(-inf)
      0x7FF802B7, 0x30502223, 0x30003087, 0xC200F353,
      0x20602823,                                              // w(NaN)
      0xC210F353, 0x20602A23,                                  // wu(NaN)
      0xBFF002B7, 0x30502223, 0x30003087, 0xC210F353,
      0x20602C23,                                              // wu(-1.0)
      0x41E002B7, 0x30502223, 0x30003087, 0xC200F353,
      0x20602E23,                                              // w(2^31)
      0xC210F353, 0x22602023,                                  // wu(2^31)
      0x41F002B7, 0x30502223, 0x30003087, 0xC210F353,
      0x22602223,                                              // wu(2^32)
    }, 45);
    CHECK_EQ(bus.load(0x200, WORD), 2);
    CHECK_EQ(bus.load(0x204, WORD), 0xFFFFFFFE); // -2
    CHECK_EQ(bus.load(0x208, WORD), 0x7FFFFFFF); // +inf -> INT32_MAX
    CHECK_EQ(bus.load(0x20C, WORD), 0x80000000); // -inf -> INT32_MIN
    CHECK_EQ(bus.load(0x210, WORD), 0x7FFFFFFF); // NaN -> INT32_MAX
    CHECK_EQ(bus.load(0x214, WORD), 0xFFFFFFFF); // NaN -> UINT32_MAX
    CHECK_EQ(bus.load(0x218, WORD), 0);          // wu(-1) -> 0
    CHECK_EQ(bus.load(0x21C, WORD), 0x7FFFFFFF); // w(2^31) saturates
    CHECK_EQ(bus.load(0x220, WORD), 0x80000000); // wu(2^31) valid
    CHECK_EQ(bus.load(0x224, WORD), 0xFFFFFFFF); // wu(2^32) saturates
  });
}

static void test_fp_d_cvt_from_int() {
  guarded("fcvt.d.w/wu", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0xFFF00293,             // t0 = 0xFFFFFFFF
      0xD20280D3, 0x20103027, // fcvt.d.w  ft1,t0 (-1)  ; fsd 0x200
      0xD21280D3, 0x20103427, // fcvt.d.wu ft1,t0 (max) ; fsd 0x208
      0x010002B7, 0x00128293, // t0 = 16777217 = 2^24+1
      0xD20280D3, 0x20103827, // fcvt.d.w ; fsd 0x210
    }, 9);
    CHECK_EQ(bus.load(0x200, WORD), 0);          // -1.0
    CHECK_EQ(bus.load(0x204, WORD), 0xBFF00000);
    // 4294967295.0 is EXACT in double (unlike float, which rounds to 2^32)
    CHECK_EQ(bus.load(0x208, WORD), 0xFFE00000);
    CHECK_EQ(bus.load(0x20C, WORD), 0x41EFFFFF);
    // 2^24+1 is exact in double; a float round-trip would drop the +1
    // and store high word 0x41700000 with LOW word 0
    CHECK_EQ(bus.load(0x210, WORD), 0x10000000);
    CHECK_EQ(bus.load(0x214, WORD), 0x41700000);
  });
}

// ------------------------------------------------ fcsr / fflags / frm views
//
// One 8-bit register (frm[7:5] | fflags[4:0]) behind three CSR addresses.
// frm reads UNSHIFTED (0..7 at bits 2:0); each view's write must leave the
// other field untouched; fcsr's reserved upper bits must not stick.

static void test_fcsr_views() {
  guarded("fcsr views", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x0FF00293, // addi  t0, x0, 0xFF
      0x00329073, // csrw  fcsr, t0        fcsr = 0xFF (frm=7, fflags=0x1F)
      0x00102373, // csrr  t1, fflags
      0x20602023, // sw    t1, 0x200(x0)   expect 0x1F
      0x00202373, // csrr  t1, frm
      0x20602223, // sw    t1, 0x204(x0)   expect 7 (unshifted!)
      0x0020D073, // csrwi frm, 1          only bits 7:5 change
      0x00302373, // csrr  t1, fcsr
      0x20602423, // sw    t1, 0x208(x0)   expect (1<<5)|0x1F = 0x3F
      0x00105073, // csrwi fflags, 0       only bits 4:0 change
      0x00302373, // csrr  t1, fcsr
      0x20602623, // sw    t1, 0x20C(x0)   expect 1<<5 = 0x20
      0xFFFFF2B7, // lui   t0, 0xFFFFF     reserved-bits probe
      0x00329073, // csrw  fcsr, t0        only low byte may stick
      0x00302373, // csrr  t1, fcsr
      0x20602823, // sw    t1, 0x210(x0)   expect 0xFFFFF000 & 0xFF = 0
    }, 16);
    CHECK_EQ(bus.load(0x200, WORD), 0x1F);
    CHECK_EQ(bus.load(0x204, WORD), 0x07);
    CHECK_EQ(bus.load(0x208, WORD), 0x3F);
    CHECK_EQ(bus.load(0x20C, WORD), 0x20);
    CHECK_EQ(bus.load(0x210, WORD), 0x00);
  });
}

// -------------------------------------------------- NaN canonicalization
//
// RISC-V arithmetic never propagates NaN payloads: any NaN result reads
// exactly 0x7FC00000 / 0x7FF8000000000000. Inputs here are sNaNs with
// payload 1 — a host that propagates would produce 0x7FC00001 instead.

static void test_nan_canonicalization() {
  guarded("canonical NaN", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x7F8002B7, 0x00128293, // t0 = 0x7F800001 (sNaN, payload 1)
      0x30502023, 0x30002087, // -> 0x300; flw ft1
      0x3F8002B7, 0x30502223, // 1.0f -> 0x304
      0x30402107,             // flw ft2
      0x0020F1D3, 0x20302027, // fadd.s  ft3,ft1,ft2 ; fsw 0x200
      0x1020F1D3, 0x20302227, // fmul.s  ft3,ft1,ft2 ; fsw 0x204
      0x1020F1C3, 0x20302427, // fmadd.s ft3,ft1,ft2,ft2 ; fsw 0x208
      0x00100293, 0x30502423, // low word 1 -> 0x308
      0x7FF002B7, 0x30502623, // sNaN.d 0x7FF00000_00000001 -> 0x308
      0x30803087,             // fld ft1
      0x30002823,             // low 0 -> 0x310
      0x3FF002B7, 0x30502A23, // 1.0 -> 0x310
      0x31003107,             // fld ft2
      0x0220F1D3, 0x20303827, // fadd.d ft3,ft1,ft2 ; fsd 0x210
      0x4010F253, 0x20402C27, // fcvt.s.d ft4,ft1 ; fsw 0x218
    }, 26);
    CHECK_EQ(bus.load(0x200, WORD), 0x7FC00000); // fadd.s
    CHECK_EQ(bus.load(0x204, WORD), 0x7FC00000); // fmul.s
    CHECK_EQ(bus.load(0x208, WORD), 0x7FC00000); // fmadd.s
    CHECK_EQ(bus.load(0x210, WORD), 0);          // fadd.d -> canonical
    CHECK_EQ(bus.load(0x214, WORD), 0x7FF80000);
    CHECK_EQ(bus.load(0x218, WORD), 0x7FC00000); // fcvt.s.d of a NaN
  });
}

// ------------------------------------------------- fflags / rounding modes
//
// The cfenv layer: every flag provoked in isolation and read back through
// csrr fflags, accumulation semantics, static and dynamic rm, exact flag
// values from the converts, and the reserved-rm illegal-instruction trap
// (which must leave rd untouched).

static void test_fflags_each_flag() {
  guarded("fflags provoke", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x3F8002B7, 0x30502023, // 1.0f -> 0x300
      0x30002223,             // 0.0f -> 0x304
      0x30002087, 0x30402107, // flw ft1, ft2
      0x00105073,             // csrwi fflags, 0
      0x1820F1D3,             // fdiv.s (1/0)
      0x00102373, 0x20602023, // csrr t1, fflags ; sw 0x200
      0x404002B7, 0x30502223, // 3.0f -> 0x304
      0x30402107,             // flw ft2
      0x00105073,             // clear
      0x1820F1D3,             // fdiv.s (1/3)
      0x00102373, 0x20602223, // fflags -> 0x204
      0x7F8002B7, 0xFFF28293, // FLT_MAX = 0x7F7FFFFF
      0x30502023, 0x30002087, // -> 0x300 ; flw ft1
      0x400002B7, 0x30502223, // 2.0f -> 0x304
      0x30402107,             // flw ft2
      0x00105073,             // clear
      0x1020F1D3,             // fmul.s (overflow)
      0x00102373, 0x20602423, // fflags -> 0x208
      0x008002B7, 0x30502023, // FLT_MIN (0x00800000) -> 0x300
      0x30002087,             // flw ft1
      0x404002B7, 0x30502223, // 3.0f -> 0x304
      0x30402107,             // flw ft2
      0x00105073,             // clear
      0x1820F1D3,             // fdiv.s (subnormal result)
      0x00102373, 0x20602623, // fflags -> 0x20C
      0x7F8002B7, 0x00128293, // sNaN 0x7F800001
      0x30502023, 0x30002087, // -> 0x300 ; flw ft1
      0x3F8002B7, 0x30502223, // 1.0f -> 0x304
      0x30402107,             // flw ft2
      0x00105073,             // clear
      0x0020F1D3,             // fadd.s (sNaN operand)
      0x00102373, 0x20602823, // fflags -> 0x210
    }, 48);
    CHECK_EQ(bus.load(0x200, WORD), 0x08); // DZ
    CHECK_EQ(bus.load(0x204, WORD), 0x01); // NX
    CHECK_EQ(bus.load(0x208, WORD), 0x05); // OF|NX
    CHECK_EQ(bus.load(0x20C, WORD), 0x03); // UF|NX
    CHECK_EQ(bus.load(0x210, WORD), 0x10); // NV
  });
}

static void test_fflags_accumulate_and_clear() {
  // flags only ever accumulate; only a CSR write clears them
  guarded("fflags accumulate", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x00105073,             // csrwi fflags, 0
      0x3F8002B7, 0x30502023, // 1.0f
      0x30002087,             // flw ft1
      0x30002223, 0x30402107, // 0.0f ; flw ft2
      0x1820F1D3,             // fdiv.s -> DZ
      0x404002B7, 0x30502223, // 3.0f
      0x30402107,             // flw ft2
      0x1820F1D3,             // fdiv.s -> NX on top
      0x00102373, 0x20602023, // fflags -> 0x200 (DZ|NX)
      0x00105073,             // csrwi fflags, 0
      0x00102373, 0x20602223, // fflags -> 0x204 (clean)
    }, 16);
    CHECK_EQ(bus.load(0x200, WORD), 0x09); // DZ|NX accumulated
    CHECK_EQ(bus.load(0x204, WORD), 0x00); // write cleared
  });
}

static void test_rounding_modes() {
  guarded("rounding modes", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x402002B7, 0x30502023, // 2.5f -> 0x300
      0x30002087,             // flw ft1
      0xC0008353, 0x20602023, // fcvt.w.s rne -> 0x200
      0xC0009353, 0x20602223, // fcvt.w.s rtz -> 0x204
      0xC000B353, 0x20602423, // fcvt.w.s rup -> 0x208
      0xC000A353, 0x20602623, // fcvt.w.s rdn -> 0x20C
      0xC02002B7, 0x30502023, // -2.5f
      0x30002087,             // flw ft1
      0xC000A353, 0x20602823, // fcvt.w.s rdn -> 0x210
      0x0021D073,             // csrwi frm, 3 (RUP)
      0x402002B7, 0x30502023, // 2.5f
      0x30002087,             // flw ft1
      0xC000F353, 0x20602A23, // fcvt.w.s DYN -> 0x214
      0x00205073,             // csrwi frm, 0
      0x3F8002B7, 0x30502023, // 1.0f
      0x30002087,             // flw ft1
      0x338002B7, 0x30502223, // 2^-24
      0x30402107,             // flw ft2
      0x0020B1D3, 0x20302C27, // fadd.s rup ; fsw 0x218
      0x0020F1D3, 0x20302E27, // fadd.s dyn (frm=RNE) ; fsw 0x21C
    }, 33);
    CHECK_EQ(bus.load(0x200, WORD), 2);          // RNE: tie to even
    CHECK_EQ(bus.load(0x204, WORD), 2);          // RTZ
    CHECK_EQ(bus.load(0x208, WORD), 3);          // RUP
    CHECK_EQ(bus.load(0x20C, WORD), 2);          // RDN
    CHECK_EQ(bus.load(0x210, WORD), 0xFFFFFFFD); // RDN(-2.5) = -3
    CHECK_EQ(bus.load(0x214, WORD), 3);          // DYN with frm=RUP
    CHECK_EQ(bus.load(0x218, WORD), 0x3F800001); // 1+2^-24 rounds up under RUP
    CHECK_EQ(bus.load(0x21C, WORD), 0x3F800000); // ... and away under RNE
  });
}

static void test_fcvt_flag_exactness() {
  // riscv-tests compares fflags EXACTLY: out-of-range must be NV alone
  // (llrint's pending NX discarded), in-range inexact must be NX alone
  guarded("fcvt flags", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x4F0002B7, 0x30502023, // 2^31 as float
      0x30002087,             // flw ft1
      0x00105073,             // clear fflags
      0xC000F353,             // fcvt.w.s (saturates)
      0x001023F3, 0x20702023, // fflags -> 0x200
      0x402002B7, 0x30502023, // 2.5f
      0x30002087,             // flw ft1
      0x00105073,             // clear
      0xC0009353,             // fcvt.w.s rtz (inexact, valid)
      0x001023F3, 0x20702223, // fflags -> 0x204
      0xBF3332B7, 0x33328293, // -0.7f = 0xBF333333
      0x30502023, 0x30002087, // -> 0x300 ; flw ft1
      0x00105073,             // clear
      0xC0109353, 0x20602423, // fcvt.wu.s rtz -> 0x208 (rounds to 0: VALID)
      0x001023F3, 0x20702623, // fflags -> 0x20C
      0x00105073,             // clear
      0xC010A353, 0x20602823, // fcvt.wu.s rdn -> 0x210 (rounds to -1: sat)
      0x001023F3, 0x20702A23, // fflags -> 0x214
    }, 28);
    CHECK_EQ(bus.load(0x200, WORD), 0x10); // exactly NV, no NX
    CHECK_EQ(bus.load(0x204, WORD), 0x01); // exactly NX
    CHECK_EQ(bus.load(0x208, WORD), 0);    // wu.rtz(-0.7) = 0, valid
    CHECK_EQ(bus.load(0x20C, WORD), 0x01); // ... with NX only
    CHECK_EQ(bus.load(0x210, WORD), 0);    // wu.rdn(-0.7) saturates to 0
    CHECK_EQ(bus.load(0x214, WORD), 0x10); // ... with NV only
  });
}

static void test_reserved_rm_traps_no_rd_write() {
  // fadd.s with static rm=101: illegal instruction (mcause 2), mepc points
  // at it, and the DESTINATION register keeps its sentinel — a trapping
  // instruction has no side effects
  guarded("reserved rm trap", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x04000293, // 0x00: addi t0, x0, 0x40    handler
      0x30529073, // 0x04: csrw mtvec, t0
      0x3F8002B7, // 0x08: lui t0, 0x3F800      1.0f
      0x30502023, // 0x0C: sw t0, 0x300(x0)
      0x30002087, // 0x10: flw ft1, 0x300(x0)
      0x400002B7, // 0x14: lui t0, 0x40000      2.0f sentinel
      0x30502223, // 0x18: sw t0, 0x304(x0)
      0x30402187, // 0x1C: flw ft3, 0x304(x0)   sentinel into DEST reg
      0x0010D1D3, // 0x20: fadd.s ft3,ft1,ft1 with rm=101 -> must trap
      0x00000013, 0x00000013, 0x00000013, 0x00000013, // nop padding
      0x00000013, 0x00000013, 0x00000013,
      0x342023F3, // 0x40: csrr t2, mcause
      0x20702223, // 0x44: sw t2, 0x204(x0)
      0x20302027, // 0x48: fsw ft3, 0x200(x0)   sentinel must survive
      0x341023F3, // 0x4C: csrr t2, mepc
      0x20702423, // 0x50: sw t2, 0x208(x0)
    }, 14);
    CHECK_EQ(bus.load(0x204, WORD), 2);          // illegal instruction
    CHECK_EQ(bus.load(0x200, WORD), 0x40000000); // rd NOT written
    CHECK_EQ(bus.load(0x208, WORD), 0x20);       // mepc = the fadd itself
  });
}

static void test_dyn_rm_invalid_frm_traps() {
  // rm=DYN with frm set to a reserved value must also be illegal
  guarded("dyn bad frm trap", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x04000293, // 0x00: addi t0, x0, 0x40
      0x30529073, // 0x04: csrw mtvec, t0
      0x0022D073, // 0x08: csrwi frm, 5         reserved frm
      0x3F8002B7, // 0x0C: lui t0, 0x3F800
      0x30502023, // 0x10: sw t0, 0x300(x0)
      0x30002087, // 0x14: flw ft1, 0x300(x0)
      0x0010F1D3, // 0x18: fadd.s ft3,ft1,ft1 (rm=dyn) -> trap
      0x00000013, 0x00000013, 0x00000013, 0x00000013, 0x00000013,
      0x00000013, 0x00000013, 0x00000013, 0x00000013, // nop padding
      0x342023F3, // 0x40: csrr t2, mcause
      0x20702023, // 0x44: sw t2, 0x200(x0)
    }, 9);
    CHECK_EQ(bus.load(0x200, WORD), 2); // illegal instruction
  });
}

// ------------------------------------------------- fmin/fmax NaN flags
//
// min/max are QUIET for qNaN inputs (no flag, return the other operand)
// but a SIGNALING NaN input raises NV — in both widths.

static void test_minmax_snan_flags() {
  guarded("minmax NV", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x7FC002B7, 0x30502023, // qNaN -> 0x300
      0x30002087,             // flw ft1
      0x3F8002B7, 0x30502223, // 1.0f -> 0x304
      0x30402107,             // flw ft2
      0x00105073,             // clear fflags
      0x282081D3,             // fmin.s (qNaN, 1.0) — quiet
      0x00102373, 0x20602023, // fflags -> 0x200: expect 0
      0x7F8002B7, 0x00128293, // sNaN 0x7F800001
      0x30502023, 0x30002087, // -> 0x300 ; flw ft1
      0x00105073,             // clear
      0x282091D3,             // fmax.s (sNaN, 1.0) — signals
      0x00102373, 0x20602223, // fflags -> 0x204: expect NV
      0x00100293, 0x30502423, // low word 1 -> 0x308
      0x7FF002B7, 0x30502623, // sNaN.d 0x7FF00000_00000001
      0x30803087,             // fld ft1
      0x30002823,             // low 0 -> 0x310
      0x3FF002B7, 0x30502A23, // 1.0 -> 0x310
      0x31003107,             // fld ft2
      0x00105073,             // clear
      0x2A2081D3,             // fmin.d (sNaN, 1.0) — signals
      0x00102373, 0x20602423, // fflags -> 0x208: expect NV
      0x30002423,             // low 0 -> 0x308
      0x7FF802B7, 0x30502623, // qNaN.d -> 0x308
      0x30803087,             // fld ft1
      0x00105073,             // clear
      0x2A2091D3,             // fmax.d (qNaN, 1.0) — quiet
      0x00102373, 0x20602623, // fflags -> 0x20C: expect 0
    }, 39);
    CHECK_EQ(bus.load(0x200, WORD), 0x00); // fmin.s qNaN: no flag
    CHECK_EQ(bus.load(0x204, WORD), 0x10); // fmax.s sNaN: NV
    CHECK_EQ(bus.load(0x208, WORD), 0x10); // fmin.d sNaN: NV
    CHECK_EQ(bus.load(0x20C, WORD), 0x00); // fmax.d qNaN: no flag
  });
}

// ------------------------------------------------------- mstatus.FS gating
//
// FS=Off must make FP instructions trap (illegal instruction) with NO side
// effects; re-enabling FS makes them work; an FP register write drives FS
// from Initial to Dirty (and SD along with it).

static void test_fs_off_gates_fp() {
  guarded("mstatus.FS", [] {
    BUS bus;
    TestRAM ram;
    run_program(bus, ram, {
      0x04000293, // 0x00: addi t0, x0, 0x40      handler
      0x30529073, // 0x04: csrw mtvec, t0
      0x3F8002B7, // 0x08: lui t0, 0x3F800        1.0f
      0x30502023, // 0x0C: sw t0, 0x300(x0)
      0x30002087, // 0x10: flw ft1, 0x300(x0)     (FS is Dirty after reset)
      0x00100293, // 0x14: addi t0, x0, 1
      0x30502223, // 0x18: sw t0, 0x304(x0)       sentinel at target
      0x000062B7, // 0x1C: lui t0, 0x6            FS mask (bits 14:13)
      0x3002B073, // 0x20: csrc mstatus, t0       FS = Off
      0x30102227, // 0x24: fsw ft1, 0x304(x0)     must TRAP, not store
      0x00000013, 0x00000013, 0x00000013, // nop padding
      0x00000013, 0x00000013, 0x00000013,
      0x342023F3, // 0x40: csrr t2, mcause
      0x20702023, // 0x44: sw t2, 0x200(x0)       RESULT0 = mcause
      0x30402383, // 0x48: lw t2, 0x304(x0)
      0x20702223, // 0x4C: sw t2, 0x204(x0)       RESULT1 = sentinel intact?
      0x000062B7, // 0x50: lui t0, 0x6
      0x3002A073, // 0x54: csrs mstatus, t0       FS = Dirty again
      0x30102227, // 0x58: fsw ft1, 0x304(x0)     now allowed
      0x30402383, // 0x5C: lw t2, 0x304(x0)
      0x20702423, // 0x60: sw t2, 0x208(x0)       RESULT2 = stored float
      0x000062B7, // 0x64: lui t0, 0x6
      0x3002B073, // 0x68: csrc mstatus, t0       FS = Off
      0x000022B7, // 0x6C: lui t0, 0x2            bit 13 only
      0x3002A073, // 0x70: csrs mstatus, t0       FS = Initial (01)
      0x30402087, // 0x74: flw ft1, 0x304(x0)     FP reg write -> FS = Dirty
      0x300023F3, // 0x78: csrr t2, mstatus
      0x20702623, // 0x7C: sw t2, 0x20C(x0)       RESULT3 = mstatus
    }, 26);
    CHECK_EQ(bus.load(0x200, WORD), 2);          // illegal instruction
    CHECK_EQ(bus.load(0x204, WORD), 1);          // fsw had NO memory effect
    CHECK_EQ(bus.load(0x208, WORD), 0x3F800000); // works with FS on
    // MPP=3 | FS=Dirty (hardware-set by the flw) | SD computed from FS
    CHECK_EQ(bus.load(0x20C, WORD), 0x80007800);
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
  test_csrrw_swap();
  test_csrrs_sets_bits();
  test_csrrc_clears_bits();
  test_csrrw_x0_still_writes();
  test_csrrwi_uses_field_not_register();
  test_csrrsi_csrrci();
  test_csr_high_address();
  test_csrrw_rd_equals_rs1();
  test_ecall_roundtrip();
  test_ebreak_cause_and_tval();
  test_mstatus_push_pop();
  test_reset_csr_values();
  test_misaligned_fetch_trap();
  test_not_taken_branch_no_trap();
  test_lr_sc_pair_and_consumption();
  test_sc_wrong_address_fails();
  test_amoswap_touches_only_rd_and_mem();
  test_fence_variants_are_nops();
  test_wfi_is_nop();
  test_flw_nan_boxing();
  test_fld_fsd_word_order();
  test_fsw_writes_four_bytes();
  test_fma_s_four_variants();
  test_fma_single_rounding();
  test_fma_sign_of_zero();
  test_fma_d_four_variants();
  test_fma_s_consumes_nan_box();
  test_fp_s_arithmetic();
  test_fp_s_min_max();
  test_fp_s_sgnj();
  test_fp_s_compare();
  test_fp_s_fclass();
  test_fp_s_cvt_to_int();
  test_fp_s_cvt_from_int_and_fmv();
  test_fp_d_arithmetic();
  test_fp_d_min_max();
  test_fp_d_sgnj();
  test_fp_d_compare();
  test_fp_d_fclass();
  test_fp_d_cvt_to_int();
  test_fp_d_cvt_from_int();
  test_fcsr_views();
  test_nan_canonicalization();
  test_fflags_each_flag();
  test_fflags_accumulate_and_clear();
  test_rounding_modes();
  test_fcvt_flag_exactness();
  test_reserved_rm_traps_no_rd_write();
  test_dyn_rm_invalid_frm_traps();
  test_minmax_snan_flags();
  test_fs_off_gates_fp();
}
