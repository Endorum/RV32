#ifndef DECODE_HPP
#define DECODE_HPP

#include <format>
#include <string>

#include "DEFS.hpp"
#include "UTILS.hpp"

enum class Op
{
  NONE,

  /* RV32I Base ISA */
  LUI,
  AUIPC,
  JAL,
  JALR,
  BEQ,
  BNE,
  BLT,
  BGE,
  BLTU,
  BGEU,
  LB,
  LH,
  LW,
  LBU,
  LHU,
  SB,
  SH,
  SW,
  ADDI,
  SLTI,
  SLTIU,
  XORI,
  ORI,
  ANDI,
  SLLI,
  SRLI,
  SRAI,
  ADD,
  SUB,
  SLL,
  SLT,
  SLTU,
  XOR,
  SRL,
  SRA,
  OR,
  AND,
  FENCE,
  FENCE_TSO,
  ECALL,
  EBREAK,

  /* RV32 Zicsr Standard Extension */
  CSRRW,
  CSRRS,
  CSRRC,
  CSRRWI,
  CSRRSI,
  CSRRCI,

  /* RV32M Standard Extension */
  MUL,
  MULH,
  MULHSU,
  MULHU,
  DIV,
  DIVU,
  REM,
  REMU,

  /* RV32A Standard Extension */
  LR,
  SC,
  AMOSWAP,
  AMOADD,
  AMOXOR,
  AMOAND,
  AMOOR,
  AMOMIN,
  AMOMAX,
  AMOMINU,
  AMOMAXU,

  /* RV32F Standard Extension */
  FLW,
  FSW,
  FMADD_S,
  FMSUB_S,
  FNMSUB_S,
  FNMADD_S,
  FADD_S,
  FSUB_S,
  FMUL_S,
  FDIV_S,
  FSQRT_S,
  FSGNJ_S,
  FSGNJN_S,
  FSGNJX_S,
  FMIN_S,
  FMAX_S,
  FCVT_W_S,
  FCVT_WU_S,
  FMV_X_W,
  FEQ_S,
  FLT_S,
  FLE_S,
  FCLASS_S,
  FCVT_S_W,
  FCVT_S_WU,
  FMV_W_X,

  /* RV32D Standart Extension */
  FLD,
  FSD,
  FMADD_D,
  FMSUB_D,
  FNMSUB_D,
  FNMADD_D,
  FADD_D,
  FSUB_D,
  FMUL_D,
  FDIV_D,
  FSQRT_D,
  FSGNJ_D,
  FSGNJN_D,
  FSGNJX_D,
  FMIN_D,
  FMAX_D,
  FCVT_S_D,
  FCVT_D_S,
  FEQ_D,
  FLT_D,
  FLE_D,
  FCLASS_D,
  FCVT_W_D,
  FCVT_WU_D,
  FCVT_D_W,
  FCVT_D_WU,

  /* RV32 Zifencei Standard Extension */
  FENCE_I,

  /* Other */
  WFI,
  MRET,

  INVALID,

};

enum class Format
{
  NONE,
  R,
  I,
  S,
  B,
  U,
  J
};

enum class BaseType : u8 {
  INVALID   = 0,
  ALU_R     = 0b0110011,
  ALU_I     = 0b0010011,
  LOAD      = 0b0000011,
  STORE     = 0b0100011,
  BRANCH    = 0b1100011,
  JAL       = 0b1101111,
  JALR      = 0b1100111,
  LUI       = 0b0110111,
  AUIPC     = 0b0010111,
  FENCE     = 0b0001111,
  SYSTEM    = 0b1110011,
  ATOMIC    = 0b0101111, 
  LOAD_FP   = 0b0000111,
  STORE_FP  = 0b0100111,
  FMADD     = 0b1000011,
  FMSUB     = 0b1000111,
  FNMSUB    = 0b1001011,
  FNMADD    = 0b1001111,
  FP_ALU    = 0b1010011,
};

typedef struct Instruction{

  // In
  u32 word;
  u32 addr;

  // Out
  u8 opcode;

  u8 rd;
  u8 rs1;
  u8 rs2;
  u8 rs3;
  
  u8 funct3;
  u8 funct7;

  i32 imm;

  // for fp ops.
  // 0b00 = S = Single = 32 Bit
  // 0b01 = D = Double = 64 Bit
  // 0b10 = H = Half   = 16 Bit
  // 0b11 = Q = Quad   = 128 Bit
  u8 width;   // (f7 & 0x3)
  u8 funct5;    // ((f7 & ~0x3) >> 2)
  u8 rm; // = f3 
  u8 fm;

  bool aq;
  bool rl;

  Format fmt;
  BaseType type;
  Op op;
  std::string mnemonic;

}Instruction;

BaseType get_type(u8 opcode);
Format get_format(BaseType type);
i32 get_immediate(Format fmt, u32 word);
Op get_op(const Instruction& instr);
std::string get_mnemonic(const Op& op);
Instruction decode(u32 word, u32 addr);
std::string disassemble(const Instruction& instr);
void validate(Instruction& instr);

#endif // DECODE_HPP
