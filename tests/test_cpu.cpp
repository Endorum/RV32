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
    // in handler: MIE(3)=0, MPIE(7)=1, MPP(12:11)=3 -> 0x1880
    CHECK_EQ(bus.load(RESULT0, WORD), 0x1880);
    // after mret: MIE restored to 1, MPIE=1, MPP stays M -> 0x1888
    CHECK_EQ(bus.load(RESULT2, WORD), 0x1888);
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
    CHECK_EQ(bus.load(RESULT0, WORD), 0x40001101); // MXL=1 (RV32) | I | M | A
    CHECK_EQ(bus.load(RESULT1, WORD), 0x40001101); // unchanged: read-only
    CHECK_EQ(bus.load(RESULT2, WORD), 0x00001800); // MPP=3, rest clear
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
  test_wfi_is_nop();
}
