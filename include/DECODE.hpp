#ifndef DECODE_HPP
#define DECODE_HPP

#include <format>
#include <string>

#include "DEFS.hpp"
#include "UTILS.hpp"

enum class InstructionFormat { NONE, R, I, S, B, U, J };

class Instruction {
public:
  Instruction() {}

  void decode();

  void set_instr_word(u32 word) { instr_word = word; }
  void set_instr_addr(u32 addr) { instr_addr = addr; }

  u32 instr_word;
  u32 instr_addr;

  u8 opc;

  // source 1 and 2, and the destination register index
  u8 rs2;
  u8 rs1;
  u8 rd;

  // funct3 and funct7 used for decoding
  u8 funct3;
  u8 funct7;

  // general imm, depending on the Instr. type
  i32 imm;

  InstructionFormat format;

  std::string mnemonic;

  std::string str() {

    return std::format(
        "word: {:08X}\naddr: {:08X}\nopc: {:02X}\nrs2: {},rs1: {} rd: "
        "{}\nfunct3: "
        "{:02X}\nfunct7: {:02X}\nimm: {:08X} = {}\nmnemonic: {}\n",
        instr_word, instr_addr, opc, rs2, rs1, rd, funct3, funct7,
        static_cast<u32>(imm), imm, mnemonic.c_str()

    );
  }

  std::string line() {

    std::string output =
        std::format("{:08X}: {:08X} \t\t", instr_addr, instr_word);

    if (mnemonic == "") {
      output += "???  ";
    } else {
      output += mnemonic + " ";
    }

    switch (format) {

    default:
      output = "UNKNOWN FORMAT";
      break;

    case InstructionFormat::R:
      output += reg_idx_str(rd) + ", ";
      output += reg_idx_str(rs1) + ", ";
      output += reg_idx_str(rs2);
      break;

    case InstructionFormat::I:

      if (opc == LOAD) {
        output += reg_idx_str(rd) + ", ";
        output += std::format("{}", imm) + "(";
        output += reg_idx_str(rs1) + ")";
        break;
      }

      output += reg_idx_str(rd) + ", ";
      output += reg_idx_str(rs1) + ", ";
      output += std::format("{}", imm);
      break;

    case InstructionFormat::S:
      output += reg_idx_str(rs2) + ", ";
      output += std::format("{}", imm);
      output += "(" + reg_idx_str(rs1) + ")";
      break;

    case InstructionFormat::B:
      output += reg_idx_str(rs1) + ", ";
      output += reg_idx_str(rs2) + ", ";
      output += std::format("{}", imm);
      break;

    case InstructionFormat::U:
      output += reg_idx_str(rd) + ", ";
      output += std::format("{}", imm);
      break;

    case InstructionFormat::J:
      output += reg_idx_str(rd) + ", ";
      output += std::format("{}", imm);
      break;
    }

    return output;
  }
};

#endif // DECODE_HPP
