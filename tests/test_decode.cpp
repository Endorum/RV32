// Decoder unit tests — no framework, just CHECK macros and an exit code.
// Build:  cmake --build build --target tests
// Run:    ./build/tests
//
// Plain assert() is not used because it vanishes under NDEBUG (release
// builds) and aborts on the first failure; these macros report every
// failure with file:line and let the run finish.

#include <string>

#include "../include/DECODE.hpp"
#include "../include/UTILS.hpp"

#include "test_common.hpp"

// ---------------------------------------------------------------- utils

static void test_utils() {
  CHECK_EQ(extract_bits(0xDEADBEEF, 0, 7), 0xEF);
  CHECK_EQ(extract_bits(0xDEADBEEF, 28, 31), 0xD);
  CHECK_EQ(extract_bits(0x00000080, 7, 11), 1);

  CHECK_EQ(sign_extend(0xFFF, 12), -1);
  CHECK_EQ(sign_extend(0x7FF, 12), 2047);
  CHECK_EQ(sign_extend(0x800, 12), -2048);
  CHECK_EQ(sign_extend(0x0, 12), 0);
}

// ------------------------------------------------------- field extraction

static void test_fields() {
  // addi t0, zero, 5
  Instruction d = decode(0x00500293, 0x80000000);
  CHECK_EQ(d.word, 0x00500293);
  CHECK_EQ(d.addr, 0x80000000);
  CHECK_EQ(d.opcode, 0x13);
  CHECK_EQ(d.rd, 5);
  CHECK_EQ(d.rs1, 0);
  CHECK_EQ(d.funct3, 0);
  CHECK(d.type == BaseType::ALU_I);
  CHECK(d.fmt == Format::I);

  // add gp, ra, sp  (R-type: all three registers)
  d = decode(0x002081B3, 0);
  CHECK_EQ(d.rd, 3);
  CHECK_EQ(d.rs1, 1);
  CHECK_EQ(d.rs2, 2);
  CHECK_EQ(d.funct7, 0x00);
  CHECK(d.fmt == Format::R);
}

// ----------------------------------------------------------- ALU R-type

static void test_alu_r() {
  CHECK(decode(0x002081B3, 0).op == Op::ADD);  // add  gp, ra, sp
  CHECK(decode(0x402081B3, 0).op == Op::SUB);  // sub  gp, ra, sp (bit 30)
  CHECK(decode(0x002091B3, 0).op == Op::SLL);  // sll  gp, ra, sp
  CHECK(decode(0x0020A1B3, 0).op == Op::SLT);  // slt  gp, ra, sp
  CHECK(decode(0x0020B1B3, 0).op == Op::SLTU); // sltu gp, ra, sp
  CHECK(decode(0x0020C1B3, 0).op == Op::XOR);  // xor  gp, ra, sp
  CHECK(decode(0x007352B3, 0).op == Op::SRL);  // srl  t0, t1, t2
  CHECK(decode(0x407352B3, 0).op == Op::SRA);  // sra  t0, t1, t2 (bit 30)
  CHECK(decode(0x0020E1B3, 0).op == Op::OR);   // or   gp, ra, sp
  CHECK(decode(0x0020F1B3, 0).op == Op::AND);  // and  gp, ra, sp

  // R-type has no immediate
  CHECK_EQ(decode(0x002081B3, 0).imm, 0);
}

// ----------------------------------------------------------- ALU I-type

static void test_alu_i() {
  Instruction d = decode(0x00500293, 0); // addi t0, zero, 5
  CHECK(d.op == Op::ADDI);
  CHECK_EQ(d.imm, 5);

  d = decode(0xFFF10093, 0); // addi ra, sp, -1
  CHECK(d.op == Op::ADDI);
  CHECK_EQ(d.imm, -1);

  d = decode(0xFFF5A513, 0); // slti a0, a1, -1
  CHECK(d.op == Op::SLTI);
  CHECK_EQ(d.imm, -1);

  d = decode(0x0015B513, 0); // sltiu a0, a1, 1
  CHECK(d.op == Op::SLTIU);
  CHECK_EQ(d.imm, 1);

  d = decode(0x0FF5F513, 0); // andi a0, a1, 255
  CHECK(d.op == Op::ANDI);
  CHECK_EQ(d.imm, 255);

  // shifts: funct7 pattern lives inside the I-immediate
  CHECK(decode(0x00331293, 0).op == Op::SLLI); // slli t0, t1, 3
  CHECK(decode(0x00335293, 0).op == Op::SRLI); // srli t0, t1, 3
  CHECK(decode(0x40335293, 0).op == Op::SRAI); // srai t0, t1, 3 (bit 30)

  // slli with a non-zero funct7 is not a valid RV32I encoding
  CHECK(decode(0x08331293, 0).op == Op::INVALID);
}

// -------------------------------------------------------------- U-type

static void test_u_type() {
  Instruction d = decode(0x123452B7, 0); // lui t0, 0x12345
  CHECK(d.op == Op::LUI);
  CHECK(d.fmt == Format::U);
  // convention: imm holds the UNSHIFTED upper immediate — execute does << 12
  CHECK_EQ(d.imm, 0x12345);

  d = decode(0x00001517, 0); // auipc a0, 1
  CHECK(d.op == Op::AUIPC);
  CHECK_EQ(d.imm, 1);
}

// ------------------------------------------------------- jumps (J / JALR)

static void test_jumps() {
  Instruction d = decode(0x008000EF, 0x80000000); // jal ra, +8
  CHECK(d.op == Op::JAL);
  CHECK(d.fmt == Format::J);
  CHECK_EQ(d.rd, 1);
  CHECK_EQ(d.imm, 8);

  d = decode(0xFFDFF06F, 0x80000000); // jal zero, -4
  CHECK(d.op == Op::JAL);
  CHECK_EQ(d.rd, 0);
  CHECK_EQ(d.imm, -4);

  d = decode(0x000280E7, 0); // jalr ra, 0(t0)
  CHECK(d.op == Op::JALR);
  CHECK_EQ(d.rd, 1);
  CHECK_EQ(d.rs1, 5);
  CHECK_EQ(d.imm, 0);

  // jalr requires funct3 == 0
  CHECK(decode(0x000290E7, 0).op == Op::INVALID);
}

// ------------------------------------------------------------- branches

static void test_branches() {
  Instruction d = decode(0x00208463, 0x80000000); // beq ra, sp, +8
  CHECK(d.op == Op::BEQ);
  CHECK(d.fmt == Format::B);
  CHECK_EQ(d.rs1, 1);
  CHECK_EQ(d.rs2, 2);
  CHECK_EQ(d.imm, 8);

  d = decode(0xFE209EE3, 0x80000000); // bne ra, sp, -4
  CHECK(d.op == Op::BNE);
  CHECK_EQ(d.imm, -4);

  // funct3 = 2 and 3 are gaps in the branch encoding space
  CHECK(decode(0x00002063, 0).op == Op::INVALID);
  CHECK(decode(0x00003063, 0).op == Op::INVALID);
}

// -------------------------------------------------------- loads / stores

static void test_loads_stores() {
  Instruction d = decode(0x00812283, 0); // lw t0, 8(sp)
  CHECK(d.op == Op::LW);
  CHECK_EQ(d.rd, 5);
  CHECK_EQ(d.rs1, 2);
  CHECK_EQ(d.imm, 8);

  d = decode(0xFFF28503, 0); // lb a0, -1(t0)
  CHECK(d.op == Op::LB);
  CHECK_EQ(d.imm, -1);

  d = decode(0x00512623, 0); // sw t0, 12(sp)
  CHECK(d.op == Op::SW);
  CHECK(d.fmt == Format::S);
  CHECK_EQ(d.rs2, 5);
  CHECK_EQ(d.rs1, 2);
  CHECK_EQ(d.imm, 12);

  d = decode(0xFE642C23, 0); // sw t1, -8(s0)
  CHECK(d.op == Op::SW);
  CHECK_EQ(d.imm, -8);

  // funct3 gaps: load f3=3, store f3=3 are not valid encodings
  CHECK(decode(0x00003003, 0).op == Op::INVALID);
  CHECK(decode(0x00003023, 0).op == Op::INVALID);
}

// ------------------------------------------------------- system / fence

static void test_system() {
  CHECK(decode(0x00000073, 0).op == Op::ECALL);
  CHECK(decode(0x00100073, 0).op == Op::EBREAK);
  CHECK(decode(0x0FF0000F, 0).op == Op::FENCE); // fence iorw, iorw

  // Zicsr: funct3 selects the op, funct3 bit 2 = "operand is the immediate"
  CHECK(decode(0x340313F3, 0).op == Op::CSRRW);   // csrrw  t2, mscratch, t1
  CHECK(decode(0x340323F3, 0).op == Op::CSRRS);   // csrrs  t2, mscratch, t1
  CHECK(decode(0x340333F3, 0).op == Op::CSRRC);   // csrrc  t2, mscratch, t1
  CHECK(decode(0x3402D073, 0).op == Op::CSRRWI);  // csrrwi x0, mscratch, 5
  CHECK(decode(0x3401E373, 0).op == Op::CSRRSI);  // csrrsi t1, mscratch, 3
  CHECK(decode(0x340173F3, 0).op == Op::CSRRCI);  // csrrci t2, mscratch, 2
  CHECK(decode(0x00004073, 0).op == Op::INVALID); // funct3=4 is reserved

  // the CSR address rides in the I-immediate; & 0xFFF must recover it even
  // after sign-extension made it negative (csr >= 0x800)
  CHECK((decode(0x340313F3, 0).imm & 0xFFF) == 0x340);
  CHECK((decode(0xBC029073, 0).imm & 0xFFF) == 0xBC0);

  // in the I-variants the rs1 field is the zimm itself, not a register index
  CHECK(decode(0x3402D073, 0).rs1 == 5);

  // PRIV subgroup (funct3=0) dispatches on funct12 — and unknown funct12
  // must be INVALID, not silently ECALL
  CHECK(decode(0x30200073, 0).op == Op::MRET);    // mret  (funct12 0x302)
  CHECK(decode(0x10500073, 0).op == Op::WFI);     // wfi   (funct12 0x105)
  CHECK(decode(0x10200073, 0).op == Op::INVALID); // sret -> S-mode, not supported
  CHECK(decode(0x30200073, 0).mnemonic == "mret");
}

// -------------------------------------------------- mnemonic / disassembly

static void test_strings() {
  CHECK(decode(0x402081B3, 0).mnemonic == "sub");
  CHECK(decode(0x00003003, 0).mnemonic == "INVALID");

  // smoke test only — exact layout is allowed to change
  std::string line = disassemble(decode(0x00500293, 0x80000000));
  CHECK(line.find("addi") != std::string::npos);
  CHECK(line.find("t0") != std::string::npos);
}

// ------------------------------------------------------- invalid opcodes

static void test_invalid_opcode_throws() {
  // Contract since the illegal-instruction trap (2026-08-13): decode() does
  // NOT throw on unknown opcodes — it yields Op::INVALID and execute raises
  // enter_trap(cause 2). Deliberately updated from the old throwing contract.
  CHECK(decode(0x00000000, 0).op == Op::INVALID);
  CHECK(decode(0xFFFFFFFF, 0).op == Op::INVALID);
}

// ---------------------------------------------------------------- entry

void run_decode_tests() {
  test_utils();
  test_fields();
  test_alu_r();
  test_alu_i();
  test_u_type();
  test_jumps();
  test_branches();
  test_loads_stores();
  test_system();
  test_strings();
  test_invalid_opcode_throws();
}
