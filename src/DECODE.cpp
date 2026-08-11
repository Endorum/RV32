#include "../include/DECODE.hpp"
#include "../include/UTILS.hpp"

BaseType get_type(u8 opcode){
  // just cast, check for invalid opcode somewhere else i guess
  // its no problem, see get_format switch ... default 
  return static_cast<BaseType>(opcode);
}

Format get_format(BaseType type){
  switch(type){
    default:
      Error<std::runtime_error>(std::format("Unknown opcode: {:02X}", (u8)type));
    
    case BaseType::ALU_R: 
      return Format::R;
    
    case BaseType::ALU_I:
    case BaseType::LOAD:
    case BaseType::JALR:
    case BaseType::SYSTEM:
    case BaseType::FENCE:
      return Format::I;

    case BaseType::STORE:
      return Format::S;

    case BaseType::BRANCH:
      return Format::B;

    case BaseType::LUI:
    case BaseType::AUIPC:
      return Format::U;

    case BaseType::JAL:
      return Format::J;

  }
}

i32 get_immediate(Format fmt, u32 word){
  switch(fmt){
    default: 
      return 0;

    case Format::R: 
      return 0;

    case Format::I:
      return sign_extend(extract_bits(word, 20, 31), 12);

    case Format::S:
      return sign_extend( (extract_bits(word, 25, 31) << 5) |
                          (extract_bits(word, 7, 11)), 12);
    
    case Format::B:
      return sign_extend( (extract_bits(word, 31, 31) << 12) |
                          (extract_bits(word, 7, 7) << 11) |
                          (extract_bits(word, 25, 30) << 5) |
                          (extract_bits(word, 8, 11) << 1), 13);

    case Format::U:
      return (i32)(extract_bits(word, 12, 31));

    case Format::J:
      return sign_extend(
              (extract_bits(word, 31, 31) << 20)  |   // imm[20] (sign bit)
              (extract_bits(word, 21, 30) << 1)   |   // imm[10:1]
              (extract_bits(word, 20, 20) << 11)  |   // imm[11]
              (extract_bits(word, 12, 19) << 12),     // imm[19:12]
            21);
  }
}

/*

001100000010 00000 000 00000 1110011

*/

Op get_op(const Instruction& instr){

  // gets indexed using funct3
  static constexpr Op ALU_R_OP[]   = {Op::ADD, Op::SLL, Op::SLT, Op::SLTU, Op::XOR, Op::SRL, Op::OR, Op::AND};
  static constexpr Op ALU_I_OP[]   = {Op::ADDI, Op::SLLI, Op::SLTI, Op::SLTIU, Op::XORI, Op::SRLI, Op::ORI, Op::ANDI};
  static constexpr Op LOAD_OP[]    = {Op::LB, Op::LH, Op::LW, Op::INVALID, Op::LBU, Op::LHU, Op::INVALID, Op::INVALID};
  static constexpr Op STORE_OP[]   = {Op::SB, Op::SH, Op::SW, Op::INVALID, Op::INVALID, Op::INVALID, Op::INVALID, Op::INVALID};
  static constexpr Op BRANCH_OP[]  = {Op::BEQ, Op::BNE, Op::INVALID, Op::INVALID, Op::BLT, Op::BGE, Op::BLTU, Op::BGEU};
  static constexpr Op SYSTEM_OP[]  = {Op::INVALID, Op::CSRRW, Op::CSRRS, Op::CSRRC, Op::INVALID, Op::CSRRWI, Op::CSRRSI, Op::CSRRCI};

  switch(instr.type){
    default: return Op::INVALID;

    case BaseType::ALU_R:{
      Op op = ALU_R_OP[instr.funct3];
      if(op == Op::ADD && instr.funct7 == 0x20) return Op::SUB;
      if(op == Op::SRL && instr.funct7 == 0x20) return Op::SRA;
      return op;
    }

    case BaseType::ALU_I:{
      Op op = ALU_I_OP[instr.funct3];
      if(op == Op::SLLI && instr.funct7 != 0x00) return Op::INVALID;
      if(op == Op::SRLI && instr.funct7 == 0x20) return Op::SRAI;
      return op;
    }

    case BaseType::LOAD:
      return LOAD_OP[instr.funct3];

    case BaseType::STORE:
      return STORE_OP[instr.funct3];

    case BaseType::BRANCH:
      return BRANCH_OP[instr.funct3];

    case BaseType::JAL:
      return Op::JAL;
    
    case BaseType::JALR:
      if(instr.funct3 != 0x0) return Op::INVALID;
      return Op::JALR;

    case BaseType::LUI:
      return Op::LUI;

    case BaseType::AUIPC: 
      return Op::AUIPC;

    case BaseType::FENCE:
      return Op::FENCE;

    case BaseType::SYSTEM:{
      if(instr.funct3 == 0x0){
        switch(instr.imm & 0xFFF){
          case 0x000: return Op::ECALL;
          case 0x001: return Op::EBREAK;
          case 0x105: return Op::WFI;
          case 0x302: return Op::MRET;
          default: return Op::INVALID;
        }
      }
      return SYSTEM_OP[instr.funct3];
    }
      
      
      
  }

}

std::string get_mnemonic(const Op& op){
  switch(op){
    case Op::NONE: return "NONE";
    
    /* RV32I Base ISA */
    case Op::LUI: return "lui";
    case Op::AUIPC: return "auipc";
    case Op::JAL: return "jal";
    case Op::JALR: return "jalr";
    case Op::BEQ: return "beq";
    case Op::BNE: return "bne";
    case Op::BLT: return "blt";
    case Op::BGE: return "bge";
    case Op::BLTU: return "bltu";
    case Op::BGEU: return "bgeu";
    case Op::LB: return "lb";
    case Op::LH: return "lh";
    case Op::LW: return "lw";
    case Op::LBU: return "lbu";
    case Op::LHU: return "lhu";
    case Op::SB: return "sb";
    case Op::SH: return "sh";
    case Op::SW: return "sw";
    case Op::ADDI: return "addi";
    case Op::SLTI: return "slti";
    case Op::SLTIU: return "sltiu";
    case Op::XORI: return "xori";
    case Op::ORI: return "ori";
    case Op::ANDI: return "andi";
    case Op::SLLI: return "slli";
    case Op::SRLI: return "srli";
    case Op::SRAI: return "srai";
    case Op::ADD: return "add";
    case Op::SUB: return "sub";
    case Op::SLL: return "sll";
    case Op::SLT: return "slt";
    case Op::SLTU: return "sltu";
    case Op::XOR: return "xor";
    case Op::SRL: return "srl";
    case Op::SRA: return "sra";
    case Op::OR: return "or";
    case Op::AND: return "and";
    case Op::FENCE: return "fence";
    case Op::ECALL: return "ecall";
    case Op::EBREAK: return "ebreak";

    /* RV32 Zicsr Standard Extension */
    case Op::CSRRW: return "csrrw";
    case Op::CSRRS: return "csrrs";
    case Op::CSRRC: return "csrrc";
    case Op::CSRRWI: return "csrrwi";
    case Op::CSRRSI: return "csrrsi";
    case Op::CSRRCI: return "csrrci";

    case Op::WFI: return "wfi";
    case Op::MRET: return "mret";

    case Op::INVALID: return "INVALID";
  }
}

Instruction decode(u32 word, u32 addr) {
  Instruction instr;
  
  instr.word = word;
  instr.addr = addr;

  instr.opcode = extract_bits(instr.word, 0, 6);
  
  instr.rd = extract_bits(instr.word, 7, 11);
  instr.rs1 = extract_bits(instr.word, 15, 19);
  instr.rs2 = extract_bits(instr.word, 20, 24);

  instr.funct3 = extract_bits(instr.word, 12, 14);
  instr.funct7 = extract_bits(instr.word, 25, 31);

  instr.type = get_type(instr.opcode);
  
  instr.fmt = get_format(instr.type);

  instr.imm = get_immediate(instr.fmt, instr.word);

  instr.op = get_op(instr);

  instr.mnemonic = get_mnemonic(instr.op);

  return instr;
}

std::string format_operands(const Instruction& instr){

  // no operands
  if(instr.op == Op::ECALL || instr.op == Op::EBREAK || instr.op == Op::FENCE) return "";

  // memory style
  if(instr.type == BaseType::LOAD || instr.type == BaseType::JALR){
    return std::format("{}, {}({})", reg_idx_str(instr.rd), instr.imm, reg_idx_str(instr.rs1));
  }

  switch(instr.fmt){
    default: return "NONE";
    case Format::R: return std::format("{}, {}, {}",      reg_idx_str(instr.rd), reg_idx_str(instr.rs1), reg_idx_str(instr.rs2));
    case Format::I: return std::format("{}, {}, {} # {:#x}",      reg_idx_str(instr.rd), reg_idx_str(instr.rs1), instr.imm, (u32)instr.imm);
    case Format::S: return std::format("{}, {}({})",      reg_idx_str(instr.rs2), instr.imm, reg_idx_str(instr.rs1));
    case Format::B: return std::format("{}, {}, {:#x} # {:+}",  reg_idx_str(instr.rs1), reg_idx_str(instr.rs2), instr.addr + instr.imm, instr.imm);
    case Format::U: return std::format("{}, {:#x}", reg_idx_str(instr.rd), (u32)instr.imm);
    case Format::J: return std::format("{}, {:#x} # {:+}", reg_idx_str(instr.rd), instr.addr + instr.imm, instr.imm);
  }

  return "???";

}

std::string disassemble(const Instruction& instr){
  return std::format("{:08X}: {:08X}    {:<8}{}", instr.addr, instr.word, instr.mnemonic, format_operands(instr));

}
