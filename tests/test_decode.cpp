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

// ------------------------------------------------------ F extension (RV32F)
// Golden words assembled with riscv64-elf-as -march=rv32ifd; register choices:
// ft1/ft2/ft3/ft4 = f1..f4, t0 = x5, t1 = x6, sp = x2, gp = x3.

static void test_rv32f() {
  Instruction d = decode(0x00812087, 0); // flw ft1, 8(sp)
  CHECK(d.op == Op::FLW);
  CHECK(d.type == BaseType::LOAD_FP);
  CHECK(d.fmt == Format::I);
  CHECK_EQ(d.rd, 1);
  CHECK_EQ(d.rs1, 2);
  CHECK_EQ(d.imm, 8);

  d = decode(0xFE21AE27, 0); // fsw ft2, -4(gp)
  CHECK(d.op == Op::FSW);
  CHECK(d.fmt == Format::S);
  CHECK_EQ(d.rs2, 2);
  CHECK_EQ(d.rs1, 3);
  CHECK_EQ(d.imm, -4);

  // R4-type: rs3 rides in bits 31:27, fmt in 26:25
  d = decode(0x203170C3, 0); // fmadd.s ft1, ft2, ft3, ft4
  CHECK(d.op == Op::FMADD_S);
  CHECK_EQ(d.rd, 1);
  CHECK_EQ(d.rs1, 2);
  CHECK_EQ(d.rs2, 3);
  CHECK_EQ(d.rs3, 4);
  CHECK(d.width == PREC::SINGLE);
  CHECK(decode(0x203170C7, 0).op == Op::FMSUB_S);  // fmsub.s
  CHECK(decode(0x203170CB, 0).op == Op::FNMSUB_S); // fnmsub.s
  CHECK(decode(0x203170CF, 0).op == Op::FNMADD_S); // fnmadd.s

  d = decode(0x003170D3, 0); // fadd.s ft1, ft2, ft3
  CHECK(d.op == Op::FADD_S);
  CHECK(d.type == BaseType::FP_ALU);
  CHECK_EQ(d.funct5, 0b00000);
  CHECK(d.width == PREC::SINGLE);
  CHECK(decode(0x083170D3, 0).op == Op::FSUB_S);   // fsub.s
  CHECK(decode(0x103170D3, 0).op == Op::FMUL_S);   // fmul.s
  CHECK(decode(0x183170D3, 0).op == Op::FDIV_S);   // fdiv.s
  CHECK(decode(0x580170D3, 0).op == Op::FSQRT_S);  // fsqrt.s ft1, ft2

  // the rm field (funct3) must not affect decode of rounded ops:
  // same fadd.s with rm=dyn (111) and rm=rtz (001)
  CHECK(decode(0x003110D3, 0).op == Op::FADD_S);   // fadd.s ..., rtz

  CHECK(decode(0x203100D3, 0).op == Op::FSGNJ_S);  // fsgnj.s
  CHECK(decode(0x203110D3, 0).op == Op::FSGNJN_S); // fsgnjn.s
  CHECK(decode(0x203120D3, 0).op == Op::FSGNJX_S); // fsgnjx.s
  CHECK(decode(0x283100D3, 0).op == Op::FMIN_S);   // fmin.s
  CHECK(decode(0x283110D3, 0).op == Op::FMAX_S);   // fmax.s

  CHECK(decode(0xC00172D3, 0).op == Op::FCVT_W_S);  // fcvt.w.s  t0, ft2
  CHECK(decode(0xC01172D3, 0).op == Op::FCVT_WU_S); // fcvt.wu.s t0, ft2
  CHECK(decode(0xD00370D3, 0).op == Op::FCVT_S_W);  // fcvt.s.w  ft1, t1
  CHECK(decode(0xD01370D3, 0).op == Op::FCVT_S_WU); // fcvt.s.wu ft1, t1

  // funct5=11100: funct3 separates fmv.x.w (000) from fclass.s (001)
  CHECK(decode(0xE00102D3, 0).op == Op::FMV_X_W);   // fmv.x.w  t0, ft2
  CHECK(decode(0xE00112D3, 0).op == Op::FCLASS_S);  // fclass.s t0, ft2
  CHECK(decode(0xF00300D3, 0).op == Op::FMV_W_X);   // fmv.w.x  ft1, t1

  CHECK(decode(0xA03122D3, 0).op == Op::FEQ_S);     // feq.s t0, ft2, ft3
  CHECK(decode(0xA03112D3, 0).op == Op::FLT_S);     // flt.s
  CHECK(decode(0xA03102D3, 0).op == Op::FLE_S);     // fle.s

  CHECK(decode(0x003170D3, 0).mnemonic == "fadd.s");
  CHECK(decode(0xE00102D3, 0).mnemonic == "fmv.x.w");
}

// ------------------------------------------------------ D extension (RV32D)

static void test_rv32d() {
  Instruction d = decode(0x01013087, 0); // fld ft1, 16(sp)
  CHECK(d.op == Op::FLD);
  CHECK_EQ(d.rd, 1);
  CHECK_EQ(d.rs1, 2);
  CHECK_EQ(d.imm, 16);

  d = decode(0xFE21BC27, 0); // fsd ft2, -8(gp)
  CHECK(d.op == Op::FSD);
  CHECK_EQ(d.rs2, 2);
  CHECK_EQ(d.rs1, 3);
  CHECK_EQ(d.imm, -8);

  CHECK(decode(0x223170C3, 0).op == Op::FMADD_D);  // fmadd.d ft1,ft2,ft3,ft4
  CHECK(decode(0x223170C7, 0).op == Op::FMSUB_D);  // fmsub.d
  CHECK(decode(0x223170CB, 0).op == Op::FNMSUB_D); // fnmsub.d
  CHECK(decode(0x223170CF, 0).op == Op::FNMADD_D); // fnmadd.d

  d = decode(0x023170D3, 0); // fadd.d ft1, ft2, ft3
  CHECK(d.op == Op::FADD_D);
  CHECK_EQ(d.funct5, 0b00000);
  CHECK(d.width == PREC::DOUBLE);
  CHECK(decode(0x0A3170D3, 0).op == Op::FSUB_D);   // fsub.d
  CHECK(decode(0x123170D3, 0).op == Op::FMUL_D);   // fmul.d
  CHECK(decode(0x1A3170D3, 0).op == Op::FDIV_D);   // fdiv.d
  CHECK(decode(0x5A0170D3, 0).op == Op::FSQRT_D);  // fsqrt.d ft1, ft2

  CHECK(decode(0x223100D3, 0).op == Op::FSGNJ_D);  // fsgnj.d
  CHECK(decode(0x223110D3, 0).op == Op::FSGNJN_D); // fsgnjn.d
  CHECK(decode(0x223120D3, 0).op == Op::FSGNJX_D); // fsgnjx.d
  CHECK(decode(0x2A3100D3, 0).op == Op::FMIN_D);   // fmin.d
  CHECK(decode(0x2A3110D3, 0).op == Op::FMAX_D);   // fmax.d

  // cross-format converts: width = destination, rs2 = source
  CHECK(decode(0x401170D3, 0).op == Op::FCVT_S_D); // fcvt.s.d ft1, ft2
  CHECK(decode(0x420100D3, 0).op == Op::FCVT_D_S); // fcvt.d.s ft1, ft2

  CHECK(decode(0xA23122D3, 0).op == Op::FEQ_D);    // feq.d t0, ft2, ft3
  CHECK(decode(0xA23112D3, 0).op == Op::FLT_D);    // flt.d
  CHECK(decode(0xA23102D3, 0).op == Op::FLE_D);    // fle.d
  CHECK(decode(0xE20112D3, 0).op == Op::FCLASS_D); // fclass.d t0, ft2

  CHECK(decode(0xC20172D3, 0).op == Op::FCVT_W_D);  // fcvt.w.d  t0, ft2
  CHECK(decode(0xC21172D3, 0).op == Op::FCVT_WU_D); // fcvt.wu.d t0, ft2
  CHECK(decode(0xD20300D3, 0).op == Op::FCVT_D_W);  // fcvt.d.w  ft1, t1
  CHECK(decode(0xD21300D3, 0).op == Op::FCVT_D_WU); // fcvt.d.wu ft1, t1

  CHECK(decode(0xC21172D3, 0).mnemonic == "fcvt.wu.d");
}

// ------------------------------------ FP reserved / RV64-only encodings

// Builds an R-type word from raw fields — for encodings no assembler will
// emit. Also serves LOAD-FP/STORE-FP probes since the field slots line up
// (funct7 = imm[11:5], rd = imm[4:0]).
static u32 fp_word(u8 funct7, u8 rs2, u8 rs1, u8 funct3, u8 rd,
                   u8 opcode = 0x53) {
  return ((u32)funct7 << 25) | ((u32)rs2 << 20) | ((u32)rs1 << 15) |
         ((u32)funct3 << 12) | ((u32)rd << 7) | opcode;
}

static void test_fp_invalid() {
  // fmv.x.d / fmv.d.x exist only on RV64 (need 64-bit x registers) — the
  // width=D twin of a valid S pattern must NOT fall through to the S op
  CHECK(decode(fp_word(0b1110001, 0, 2, 0, 5), 0).op == Op::INVALID);
  CHECK(decode(fp_word(0b1111001, 0, 6, 0, 1), 0).op == Op::INVALID);

  // fmt = H (10) and Q (11): unsupported precisions on every funct5
  CHECK(decode(fp_word(0b0000010, 3, 2, 7, 1), 0).op == Op::INVALID); // fadd.h
  CHECK(decode(fp_word(0b0000011, 3, 2, 7, 1), 0).op == Op::INVALID); // fadd.q
  CHECK(decode(fp_word(0b0101110, 0, 2, 7, 1), 0).op == Op::INVALID); // fsqrt.h

  // cross-format converts: same-format (S->S, D->D) is reserved
  CHECK(decode(fp_word(0b0100000, 0, 2, 0, 1), 0).op == Op::INVALID);
  CHECK(decode(fp_word(0b0100001, 1, 2, 0, 1), 0).op == Op::INVALID);

  // rs2 is a fixed sub-opcode for the unary ops — nonzero garbage must fail
  CHECK(decode(fp_word(0b0101100, 1, 2, 7, 1), 0).op == Op::INVALID); // fsqrt.s
  CHECK(decode(fp_word(0b1110000, 1, 2, 0, 5), 0).op == Op::INVALID); // fmv.x.w
  CHECK(decode(fp_word(0b1111000, 1, 6, 0, 1), 0).op == Op::INVALID); // fmv.w.x
  CHECK(decode(fp_word(0b1100000, 2, 2, 7, 5), 0).op == Op::INVALID); // fcvt.w.s
  CHECK(decode(fp_word(0b1101000, 2, 6, 7, 1), 0).op == Op::INVALID); // fcvt.s.w

  // funct3 gaps inside valid funct5 groups
  CHECK(decode(fp_word(0b0010000, 3, 2, 3, 1), 0).op == Op::INVALID); // fsgnj f3=3
  CHECK(decode(fp_word(0b0010100, 3, 2, 2, 1), 0).op == Op::INVALID); // fmin  f3=2
  CHECK(decode(fp_word(0b1010000, 3, 2, 3, 5), 0).op == Op::INVALID); // feq   f3=3
  CHECK(decode(fp_word(0b1110000, 0, 2, 2, 5), 0).op == Op::INVALID); // fclass f3=2

  // funct5 with no instruction assigned in F/D
  CHECK(decode(fp_word(0b0011000, 3, 2, 0, 1), 0).op == Op::INVALID);

  // LOAD-FP/STORE-FP width gaps: H (001) and Q (100) are not implemented
  CHECK(decode(fp_word(0, 0, 2, 0b001, 1, 0x07), 0).op == Op::INVALID); // flh
  CHECK(decode(fp_word(0, 0, 2, 0b100, 1, 0x07), 0).op == Op::INVALID); // flq
  CHECK(decode(fp_word(0, 2, 3, 0b001, 0, 0x27), 0).op == Op::INVALID); // fsh

  // R4-type with fmt = H: funct7 = rs3<<2 | fmt
  CHECK(decode(fp_word((4 << 2) | 2, 3, 2, 7, 1, 0x43), 0).op == Op::INVALID);
  CHECK(decode(fp_word((4 << 2) | 3, 3, 2, 7, 1, 0x4F), 0).op == Op::INVALID);
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
  test_rv32f();
  test_rv32d();
  test_fp_invalid();
}
