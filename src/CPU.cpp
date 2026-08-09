

#include "../include/CPU.hpp"
#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include <cstring>

void CPU::IF() {
  if (T_config.B_debug)
    printf("IF: ");
  memset((void *)&instr_cache, 0, sizeof(instr_cache));

  u32 instr_addr = pc;

  u32 instr = bus.read(instr_addr, WORD);

  instr_cache.set_instr_word(instr);
  instr_cache.set_instr_addr(pc);

  if (jump) {
    jump = false;
  } else {
    pc += 4;
  }

  if (T_config.B_debug)
    printf("word: %08X, addr: %08X\n", instr, instr_addr);
}

void CPU::ID() {
  if (T_config.B_debug)
    printf("ID: ");
  instr_cache.decode();

  // repread from the registers
  src1_value = getReg(instr_cache.rs1);
  src2_value = getReg(instr_cache.rs2);
  if (T_config.B_debug)
    printf("src1: %08X, src2: %08X\n", src1_value, src2_value);
}

void CPU::EX() {

  if (T_config.B_debug)
    printf("EX: ");
  alu_result = -1;

  switch (instr_cache.format) {
  default:
    break;

  case InstructionFormat::R:
    alu_result = alu_r_type(src1_value, src2_value, &instr_cache);
    break;

  case InstructionFormat::I:
    alu_result = alu_i_type(src1_value, &instr_cache);
    break;

  case InstructionFormat::S:
    alu_result = src1_value + instr_cache.imm;
    break;

  case InstructionFormat::B:
    branch = evaluate_branch(src1_value, src2_value, &instr_cache);
    alu_result = instr_cache.instr_addr + instr_cache.imm;
    break;

  case InstructionFormat::U:
    alu_result = compute_u_type(&instr_cache);
    break;

  case InstructionFormat::J:
    alu_result = static_cast<i32>(instr_cache.instr_addr) + instr_cache.imm;
    instr_cache.mnemonic =
        "jal"; // at least for RV32I jal is the only instr with type J
    break;
  }

  jump = false;

  switch (instr_cache.opc) {
  default:
    break;

  case BRANCH:
    if (branch) {
      // pc = alu_result;
      // jump = true;
    }
    break;

  case JAL:
  case JALR:
    // pc = alu_result;
    // jump = true;
    break;
  }

  link_value = instr_cache.instr_addr + 4;
  if (T_config.B_debug)
    printf("alu_result: %08X, pc: %08X, jump: %d, link_value: %08X\n",
           alu_result, pc, jump, link_value);
}

void CPU::MEM() {
  if (T_config.B_debug)
    printf("MEM: ");
  mem_result = alu_result;

  if (instr_cache.opc == LOAD) { // load
    switch (instr_cache.funct3) {

    default:
      break;

    case 0x0:
      instr_cache.mnemonic = "lb";
      mem_result = static_cast<i8>(read(alu_result, BYTE));
      break;
    case 0x1:
      instr_cache.mnemonic = "lh";
      mem_result = static_cast<i16>(read(alu_result, HALF));
      break;
    case 0x2:
      instr_cache.mnemonic = "lw";
      mem_result = static_cast<i32>(read(alu_result, WORD));
      break;
    case 0x4:
      instr_cache.mnemonic = "lbu";
      mem_result = static_cast<u8>(read(alu_result, BYTE));
      break;
    case 0x5:
      instr_cache.mnemonic = "lhu";
      mem_result = static_cast<u16>(read(alu_result, HALF));
      break;
    }
  } else if (instr_cache.opc == STORE) {

    switch (instr_cache.funct3) {

    default:
      break;

    case 0x0:
      instr_cache.mnemonic = "sb";
      write(alu_result, BYTE, static_cast<u8>(src2_value));
      break;
    case 0x1:
      instr_cache.mnemonic = "sh";
      write(alu_result, HALF, static_cast<u16>(src2_value));
      break;
    case 0x2:
      instr_cache.mnemonic = "sw";
      write(alu_result, WORD, static_cast<u32>(src2_value));
      break;
    }
  }

  if (T_config.B_debug)
    printf("MEM: mem_result: %08X\n", mem_result);
}

void CPU::WB() {
  if (T_config.B_debug)
    printf("WB: ");
  u32 rd = instr_cache.rd;

  if (rd == 0)
    return;

  switch (instr_cache.opc) {

  default:
    printf("ERROR: Unknown opcode in WB: %02X\n", instr_cache.opc);
    exit(1);

  case ALU_R:
  case ALU_I:
  case LOAD:
  case LUI:
  case AUIPC:
    if (T_config.B_debug)
      printf("write %08X to rd: %08X\n", mem_result, rd);
    setReg(rd, mem_result);
    if (T_config.B_debug)
      printf("new value in x%d: %08X\n", rd, getReg(rd));
    break;

  case JAL:
  case JALR:
    setReg(rd, link_value);
    break;

  case STORE:
  case BRANCH:
  case SYSTEM:
    break;
  }

  // printf("rd: %d\n", rd);
}

u32 alu_r_type(u32 src1, u32 src2, Instruction *instr) {

  i32 s_src1 = static_cast<i32>(src1);
  i32 s_src2 = static_cast<i32>(src2);

  u32 shmt = instr->rs2 & 0x1F;

  // RV32I
  if (instr->funct7 == 0x00) {
    switch (instr->funct3) {
    default:
      break;

    case 0x0:
      instr->mnemonic = "add";
      return src1 + src2;
    case 0x1:
      instr->mnemonic = "sll";
      return src1 << shmt;
    case 0x2:
      instr->mnemonic = "slt";
      return (s_src1 < s_src2) ? 1 : 0;
    case 0x3:
      instr->mnemonic = "sltu";
      return (src1 < src2) ? 1 : 0;
    case 0x4:
      instr->mnemonic = "xor";
      return src1 ^ src2;
    case 0x5:
      instr->mnemonic = "srl";
      return src1 >> shmt;
    case 0x6:
      instr->mnemonic = "or";
      return src1 | src2;
    case 0x7:
      instr->mnemonic = "and";
      return src1 & src2;
    }
  } else if (instr->funct7 == 0x20) {
    switch (instr->funct3) {
    default:
      break;

    case 0x0:
      instr->mnemonic = "sub";
      return src1 - src2;
    case 0x5:
      instr->mnemonic = "sra";
      return s_src1 >> shmt;
    }
  }

  printf("ERROR: Unknown funct7 value in alu_r_type: %02X\n", instr->funct7);
  exit(1);

  return 0;
}

u32 alu_i_type(u32 src1, Instruction *instr) {

  i32 imm = instr->imm;
  u32 uimm = static_cast<u32>(instr->imm);

  i32 s_src1 = static_cast<i32>(src1);

  u32 shmt = imm & 0x1F;

  u32 funct7 = (imm >> 5) & 0x7F;

  switch (instr->opc) {
  default:
    break;

  case ALU_I: // Arithmetic Imm

    switch (instr->funct3) {
    default:
      break;

    case 0x0:
      instr->mnemonic = "addi";
      return src1 + imm;
    case 0x1:
      instr->mnemonic = "slli";
      return src1 << shmt;
    case 0x2:
      instr->mnemonic = "slti";
      return (i32(src1) < imm) ? 1 : 0;
    case 0x3:
      instr->mnemonic = "sltiu";
      return (src1 < uimm) ? 1 : 0;
    case 0x4:
      instr->mnemonic = "xori";
      return src1 ^ imm;

    case 0x5:
      if (funct7 == 0x20) { // 0b0100000
        instr->mnemonic = "srai";
        return (i32(src1)) >> shmt;
      } else {
        instr->mnemonic = "srli";
        return src1 >> shmt;
      }

    case 0x6:
      instr->mnemonic = "ori";
      return src1 | imm;
    case 0x7:
      instr->mnemonic = "andi";
      return src1 & imm;
    }

    break;

  case LOAD:
    switch (instr->funct3) {
    case 0x0:
      instr->mnemonic = "lb";
      return s_src1 + imm;
    case 0x1:
      instr->mnemonic = "lh";
      return s_src1 + imm;
    case 0x2:
      instr->mnemonic = "lw";
      return s_src1 + imm;
    case 0x4:
      instr->mnemonic = "lbu";
      return src1 + imm;
    case 0x5:
      instr->mnemonic = "lhu";
      return src1 + imm;

    default:
      printf("Invalid read funct3\n");
      exit(1);
    }

  case JALR:
    instr->mnemonic = "jalr";
    return (s_src1 + imm) & ~1; // jalr target

  // System instr
  case SYSTEM: // ecall or ebreak
    if (imm == 0x0) {
      instr->mnemonic = "ecall";
    } else if (imm == 0x1) {
      instr->mnemonic = "ebreak";
    }

    if (imm == 0x105) {
      instr->mnemonic = "wfi";
    }
    // nothing implemented yet
    return 0;

  case FENCE: // does nothing for now
    instr->mnemonic = "fence";
    return 0;
  }

  printf("ERROR: Unknown opcode value in alu_i_type: %02X\n", instr->opc);
  exit(1);

  return 0;
}

bool evaluate_branch(u32 src1, u32 src2, Instruction *instr) {

  i32 sA = static_cast<i32>(src1);
  i32 sB = static_cast<i32>(src2);

  u32 uA = src1;
  u32 uB = src2;

  switch (instr->funct3) {
  default:
    break;

  case 0x0:
    instr->mnemonic = "beq";
    return sA == sB;
  case 0x1:
    instr->mnemonic = "bne";
    return sA != sB;
  case 0x4:
    instr->mnemonic = "blt";
    return sA < sB;
  case 0x5:
    instr->mnemonic = "bge";
    return sA >= sB;
  case 0x6:
    instr->mnemonic = "bltu";
    return uA < uB;
  case 0x7:
    instr->mnemonic = "bgeu";
    return uA >= uB;
  }

  printf("ERROR: Unknown funct3 value in evaluate_branch: %02X\n",
         instr->funct3);
  exit(1);

  return false;
}

u32 compute_u_type(Instruction *instr) {

  switch (instr->opc) {
  default:
    break;

  case LUI:
    instr->mnemonic = "lui";
    return instr->imm << 12; // read Upper imm
  case AUIPC:
    instr->mnemonic = "auipc";
    return instr->instr_addr + (instr->imm << 12); // add upper imm to pc
  }

  printf("ERROR: Unknown opcode value in compute_u_type: %02X\n", instr->opc);
  exit(1);

  return 0;
}
