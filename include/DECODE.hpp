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
  ECALL,
  EBREAK,

  /* RV32 Zicsr Standard Extension */
  CSRRW,
  CSRRS,
  CSRRC,
  CSRRWI,
  CSRRSI,
  CSRRCI,

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
    ALU_R   = 0b0110011,
    ALU_I   = 0b0010011,
    LOAD    = 0b0000011,
    STORE   = 0b0100011,
    BRANCH  = 0b1100011,
    JAL     = 0b1101111,
    JALR    = 0b1100111,
    LUI     = 0b0110111,
    AUIPC   = 0b0010111,
    FENCE   = 0b0001111,
    SYSTEM  = 0b1110011,
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
  
  u8 funct3;
  u8 funct7;

  i32 imm;

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

#endif // DECODE_HPP
