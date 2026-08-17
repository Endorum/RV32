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
      return Format::NONE; // to generate a Op::invalid to handle as trap instead of exc
      //Error<std::runtime_error>(std::format("Unknown opcode: {:02X}", (u8)type));
    
    case BaseType::ALU_R: 
    case BaseType::ATOMIC:
    case BaseType::FP_ALU:
    case BaseType::FMADD:
    case BaseType::FMSUB:
    case BaseType::FNMSUB:
    case BaseType::FNMADD:
      return Format::R;
    
    case BaseType::ALU_I:
    case BaseType::LOAD:
    case BaseType::JALR:
    case BaseType::SYSTEM:
    case BaseType::FENCE:
    case BaseType::LOAD_FP:
      return Format::I;

    case BaseType::STORE:
    case BaseType::STORE_FP:
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
  static constexpr Op ALU_R_OP[]          = {Op::ADD, Op::SLL, Op::SLT, Op::SLTU, Op::XOR, Op::SRL, Op::OR, Op::AND}; // f7 = 0x0 or f7 = 0x20
  static constexpr Op ALU_R_OP_M_EXT[]    = {Op::MUL, Op::MULH, Op::MULHSU, Op::MULHU, Op::DIV, Op::DIVU, Op::REM, Op::REMU}; // f7 = 0x1
  static constexpr Op ALU_I_OP[]          = {Op::ADDI, Op::SLLI, Op::SLTI, Op::SLTIU, Op::XORI, Op::SRLI, Op::ORI, Op::ANDI};
  static constexpr Op LOAD_OP[]           = {Op::LB, Op::LH, Op::LW, Op::INVALID, Op::LBU, Op::LHU, Op::INVALID, Op::INVALID};
  static constexpr Op STORE_OP[]          = {Op::SB, Op::SH, Op::SW, Op::INVALID, Op::INVALID, Op::INVALID, Op::INVALID, Op::INVALID};
  static constexpr Op BRANCH_OP[]         = {Op::BEQ, Op::BNE, Op::INVALID, Op::INVALID, Op::BLT, Op::BGE, Op::BLTU, Op::BGEU};
  static constexpr Op SYSTEM_OP[]         = {Op::INVALID, Op::CSRRW, Op::CSRRS, Op::CSRRC, Op::INVALID, Op::CSRRWI, Op::CSRRSI, Op::CSRRCI};

  // helber for selecting the correct FP variant based on precision / width
  auto sel = [&](Op s, Op d){
    if(instr.width == PREC::SINGLE) return s;
    if(instr.width == PREC::DOUBLE) return d;
    // ...
    return Op::INVALID;
  };

  switch(instr.type){
    default: return Op::INVALID;

    case BaseType::ALU_R:{
      
      
      if(instr.funct7 == 0x0){
        // I Ext Base
        return ALU_R_OP[instr.funct3];
        
      }else if(instr.funct7 == 0x20){
        // I Ext Alt.
        Op op = ALU_R_OP[instr.funct3];
        if(op == Op::ADD && instr.funct7 == 0x20) return Op::SUB;
        if(op == Op::SRL && instr.funct7 == 0x20) return Op::SRA;
        return Op::INVALID;

      }else if(instr.funct7 == 0x01){
        // M Ext 
        return ALU_R_OP_M_EXT[instr.funct3];

      }

      else return Op::INVALID;
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
      
      if(instr.funct3 == 0b000){
        if(instr.fm == 0b1000) return Op::FENCE_TSO;
        return Op::FENCE;
      }

      if(instr.funct3 == 0b001) return Op::FENCE_I;
      return Op::INVALID;

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

    case BaseType::ATOMIC:{
      
      if(instr.funct3 != 0b010) return Op::INVALID;

      switch(instr.funct7 >> 2){
        default: return Op::INVALID;
        case 0b00000: return Op::AMOADD;
        case 0b00001: return Op::AMOSWAP;
        case 0b00010: 
          if(instr.rs2 == 0x0) return Op::LR; 
          else return Op::INVALID;
        case 0b00011: return Op::SC;
        case 0b00100: return Op::AMOXOR;
        case 0b01000: return Op::AMOOR;
        case 0b01100: return Op::AMOAND;
        case 0b10000: return Op::AMOMIN;
        case 0b10100: return Op::AMOMAX;
        case 0b11000: return Op::AMOMINU;
        case 0b11100: return Op::AMOMAXU;
      }

    }

    case BaseType::LOAD_FP:
      if(instr.funct3 == 0b010) 
        return Op::FLW;
      else if(instr.funct3 == 0b011)
        return Op::FLD;
      
      return Op::INVALID;
    
    case BaseType::STORE_FP:
      if(instr.funct3 == 0b010)
        return Op::FSW;
      else if(instr.funct3 == 0b011)
        return Op::FSD;

      return Op::INVALID;
    
    case BaseType::FMADD:
      return sel(Op::FMADD_S, Op::FMADD_D);

    case BaseType::FMSUB:
      return sel(Op::FMSUB_S, Op::FMSUB_D);

    case BaseType::FNMSUB:
      return sel(Op::FNMSUB_S, Op::FNMSUB_D);

    case BaseType::FNMADD:
      return sel(Op::FNMADD_S, Op::FNMADD_D);

    case BaseType::FP_ALU:
      switch(instr.funct5){
        default: return Op::INVALID;
        case 0b00000: return sel(Op::FADD_S, Op::FADD_D);
        case 0b00001: return sel(Op::FSUB_S, Op::FSUB_D);
        case 0b00010: return sel(Op::FMUL_S, Op::FMUL_D);
        case 0b00011: return sel(Op::FDIV_S, Op::FDIV_D);
        case 0b01011: 
          if(instr.rs2 == 0b0)
            return sel(Op::FSQRT_S, Op::FSQRT_D);
          return Op::INVALID;

        case 0b00100: 
          if(instr.funct3 == 0b000) return sel(Op::FSGNJ_S, Op::FSGNJ_D);
          if(instr.funct3 == 0b001) return sel(Op::FSGNJN_S, Op::FSGNJN_D);
          if(instr.funct3 == 0b010) return sel(Op::FSGNJX_S, Op::FSGNJX_D);
          return Op::INVALID;

        case 0b00101:
          if(instr.funct3 == 0b000) return sel(Op::FMIN_S, Op::FMIN_D);
          if(instr.funct3 == 0b001) return sel(Op::FMAX_S, Op::FMAX_D);
          return Op::INVALID;

        case 0b11000:
          if(instr.rs2 == 0b00000) return sel(Op::FCVT_W_S, Op::FCVT_W_D);
          if(instr.rs2 == 0b00001) return sel(Op::FCVT_WU_S, Op::FCVT_WU_D);
          return Op::INVALID;

        case 0b11100:
          if(instr.rs2 == 0b00000 && 
             instr.funct3 == 0b000 && 
             instr.width == PREC::SINGLE) return Op::FMV_X_W;
          if(instr.rs2 == 0b00000 && instr.funct3 == 0b001) return sel(Op::FCLASS_S, Op::FCLASS_D);
          return Op::INVALID;

        case 0b10100:
          if(instr.funct3 == 0b000) return sel(Op::FLE_S, Op::FLE_D);
          if(instr.funct3 == 0b001) return sel(Op::FLT_S, Op::FLT_D);
          if(instr.funct3 == 0b010) return sel(Op::FEQ_S, Op::FEQ_D);
          return Op::INVALID;

        case 0b11010:
          if(instr.rs2 == 0b00000) return sel(Op::FCVT_S_W, Op::FCVT_D_W);
          if(instr.rs2 == 0b00001) return sel(Op::FCVT_S_WU, Op::FCVT_D_WU);
          return Op::INVALID;

        case 0b11110:
          if(instr.rs2 == 0b00000 && 
             instr.funct3 == 0b000 && 
             instr.width == PREC::SINGLE) return Op::FMV_W_X;
          return Op::INVALID;

        case 0b01000:
          if(instr.width == PREC::SINGLE && instr.rs2 == 0b00001) return Op::FCVT_S_D;
          if(instr.width == PREC::DOUBLE && instr.rs2 == 0b00000) return Op::FCVT_D_S;
          return Op::INVALID;
        
      }
      return Op::INVALID;
      
    
      
  }


  return Op::INVALID;

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
    case Op::FENCE_TSO: return "fence.tso";
    case Op::ECALL: return "ecall";
    case Op::EBREAK: return "ebreak";

    /* RV32 Zicsr Standard Extension */
    case Op::CSRRW: return "csrrw";
    case Op::CSRRS: return "csrrs";
    case Op::CSRRC: return "csrrc";
    case Op::CSRRWI: return "csrrwi";
    case Op::CSRRSI: return "csrrsi";
    case Op::CSRRCI: return "csrrci";

    /* RV32M Standard Extension */
    case Op::MUL: return "mul";
    case Op::MULH: return "mulh";
    case Op::MULHSU: return "mulhsu";
    case Op::MULHU: return "mulhu";
    case Op::DIV: return "div";
    case Op::DIVU: return "divu";
    case Op::REM: return "rem";
    case Op::REMU: return "remu";

    /* RV32A Standard Extension */
    case Op::LR: return "lr";
    case Op::SC: return "sc";
    case Op::AMOSWAP: return "amoswap";
    case Op::AMOADD: return "amoadd";
    case Op::AMOXOR: return "amoxor";
    case Op::AMOAND: return "amoand";
    case Op::AMOOR: return "amoor";
    case Op::AMOMIN: return "amomin";
    case Op::AMOMAX: return "amomax";
    case Op::AMOMINU: return "amominu";
    case Op::AMOMAXU: return "amomaxu";

    /* RV32F Standard Extension */
    case Op::FLW: return "flw";
    case Op::FSW: return "fsw";
    case Op::FMADD_S: return "fmadd.s";
    case Op::FMSUB_S: return "fmsub.s";
    case Op::FNMSUB_S: return "fnmsub.s";
    case Op::FNMADD_S: return "fnmadd.s";
    case Op::FADD_S: return "fadd.s";
    case Op::FSUB_S: return "fsub.s";
    case Op::FMUL_S: return "fmul.s";
    case Op::FDIV_S: return "fdiv.s";
    case Op::FSQRT_S: return "fsqrt.s";
    case Op::FSGNJ_S: return "fsgnj.s";
    case Op::FSGNJN_S: return "fsgnjn.s";
    case Op::FSGNJX_S: return "fsgnjx.s";
    case Op::FMIN_S: return "fmin.s";
    case Op::FMAX_S: return "fmax.s";
    case Op::FCVT_W_S: return "fcvt.w.s";
    case Op::FCVT_WU_S: return "fcvt.wu.s";
    case Op::FMV_X_W: return "fmv.x.w";
    case Op::FEQ_S: return "feq.s";
    case Op::FLT_S: return "flt.s";
    case Op::FLE_S: return "fle.s";
    case Op::FCLASS_S: return "fclass.s";
    case Op::FCVT_S_W: return "fcvt.s.w";
    case Op::FCVT_S_WU: return "fcvt.s.wu";
    case Op::FMV_W_X: return "fmv.w.x";

    /* RV32D Standart Extension */
    case Op::FLD: return "fld";
    case Op::FSD: return "fsd";
    case Op::FMADD_D: return "fmadd.d";
    case Op::FMSUB_D: return "fmsub.d";
    case Op::FNMSUB_D: return "fnmsub.d";
    case Op::FNMADD_D: return "fnmadd.d";
    case Op::FADD_D: return "fadd.d";
    case Op::FSUB_D: return "fsub.d";
    case Op::FMUL_D: return "fmul.d";
    case Op::FDIV_D: return "fdiv.d";
    case Op::FSQRT_D: return "fsqrt.d";
    case Op::FSGNJ_D: return "fsgnj.d";
    case Op::FSGNJN_D: return "fsgnjn.d";
    case Op::FSGNJX_D: return "fsgnjx.d";
    case Op::FMIN_D: return "fmin.d";
    case Op::FMAX_D: return "fmax.d";
    case Op::FCVT_S_D: return "fcvt.s.d";
    case Op::FCVT_D_S: return "fcvt.d.s";
    case Op::FEQ_D: return "feq.d";
    case Op::FLT_D: return "flt.d";
    case Op::FLE_D: return "fle.d";
    case Op::FCLASS_D: return "fclass.d";
    case Op::FCVT_W_D: return "fcvt.w.d";
    case Op::FCVT_WU_D: return "fcvt.wu.d";
    case Op::FCVT_D_W: return "fcvt.d.w";
    case Op::FCVT_D_WU: return "fcvt.d.wu";

    /* RV32 Zifencei Standard Extension */
    case Op::FENCE_I: return "fence.i";

    /* Other */
    case Op::WFI: return "wfi";
    case Op::MRET: return "mret";

    case Op::INVALID: return "INVALID";
  }
}

void validate(Instruction& instr){
  if(instr.op <= Op::NONE || instr.op >= Op::INVALID){
    instr.op = Op::INVALID;
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
  instr.rs3 = extract_bits(instr.word, 27, 31);

  instr.funct3 = extract_bits(instr.word, 12, 14);
  instr.funct7 = extract_bits(instr.word, 25, 31);
  

  // for A extension
  instr.aq = extract_bits(instr.word, 26, 26);
  instr.rl = extract_bits(instr.word, 25, 25);

  // for F/D ext.
  instr.funct5 = (instr.funct7 & ~0x3) >> 2;
  instr.width = (PREC)(instr.funct7 & 0x3);
  instr.rm = instr.funct3;

  // for FENCE
  instr.fm = extract_bits(instr.word, 28, 31);

  instr.type = get_type(instr.opcode);
  
  instr.fmt = get_format(instr.type);

  instr.imm = get_immediate(instr.fmt, instr.word);

  

  instr.op = get_op(instr);

  instr.mnemonic = get_mnemonic(instr.op);

  validate(instr);

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
  std::string mnemonic = get_mnemonic(instr.op);
  return std::format("{:08X}: {:08X}    {:<8}{}", instr.addr, instr.word, mnemonic, format_operands(instr));

}
