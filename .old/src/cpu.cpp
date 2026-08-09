#include "cpu.hpp"

#include "decode.hpp"
#include "execute.hpp"

void RV32::debug() {
  printf("instruction: \n");
  instr_cache.print();
  rf.print();
  printf("PC: %08X\n", pc);
}

void RV32::IF() {

  memset(&instr_cache, 0, sizeof(instr_cache));

  u32 instr_addr = pc;

  u32 instr = bus->read(instr_addr, WORD);

  instr_cache.set_instr_word(instr);
  instr_cache.set_instr_addr(pc);

  if (jump) {
    jump = false;
  } else {
    pc += 4;
  }

  printf("IF: instr_word: %08X instr_addr: %08X\n", instr, instr_addr);
}

void RV32::ID() {

  instr_cache.decode();

  // preload from the registers
  src1_value = rf.read_reg(instr_cache.rs1);
  src2_value = rf.read_reg(instr_cache.rs2);

  printf("ID: decoded: \n");
  instr_cache.print();

  printf("    src1_value: %08X\n", src1_value);
  printf("    src2_value: %08X\n", src2_value);
}

void RV32::EX() {
  printf("EX\n");
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
    break;
  }

  jump = false;

  switch (instr_cache.opc) {
  default:
    break;

  case BRANCH:
    if (branch) {
      pc = alu_result;
      jump = true;
    }
    break;

  case JAL:
  case JALR:
    pc = alu_result;
    jump = true;
    break;
  }

  link_value = instr_cache.instr_addr + 4;

  printf("EX: alu_result: %08X link_value: %08X, jump: %d, branch: %d\n",
         alu_result, link_value, jump, link_value);
}

void RV32::MEM() {
  printf("MEM\n");
  mem_result = alu_result;

  if (instr_cache.opc == LOAD) { // load
    switch (instr_cache.funct3) {

    default:
      break;

    case 0x0:
      mem_result = static_cast<i8>(load(alu_result, BYTE));
      break;
    case 0x1:
      mem_result = static_cast<i16>(load(alu_result, HALF));
      break;
    case 0x2:
      mem_result = static_cast<i32>(load(alu_result, WORD));
      break;
    case 0x4:
      mem_result = static_cast<u8>(load(alu_result, BYTE));
      break;
    case 0x5:
      mem_result = static_cast<u16>(load(alu_result, HALF));
      break;
    }
  } else if (instr_cache.opc == STORE) {

    switch (instr_cache.funct3) {

    default:
      break;

    case 0x0:
      instr_cache.mnemonic = "sb";
      store(alu_result, BYTE, static_cast<u8>(src2_value));
      break;
    case 0x1:
      instr_cache.mnemonic = "sh";
      store(alu_result, HALF, static_cast<u16>(src2_value));
      break;
    case 0x2:
      instr_cache.mnemonic = "sw";
      store(alu_result, WORD, static_cast<u32>(src2_value));
      break;
    }
  }

  printf("MEM: mem_result: %08X\n", mem_result);
}

void RV32::WB() {
  printf("WB\n");
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
    printf("write %08X to rd: %08X\n", mem_result, rd);
    rf.write_reg(rd, mem_result);
    printf("new value in x%d: %08X\n", rd, rf.read_reg(rd));
    break;

  case JAL:
  case JALR:
    rf.write_reg(rd, link_value);
    break;

  case STORE:
  case BRANCH:
  case ECALL:
    break;
  }
}
