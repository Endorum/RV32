#include "../include/DECODE.hpp"
#include "../include/UTILS.hpp"

InstructionFormat get_format(u8 opc) {
  switch (opc) {
  default:
    printf("ERROR: Unknown opcode: %02X\n", opc);
    exit(1);
    break;

  case ALU_R:
    return InstructionFormat::R;

  case ALU_I:
  case LOAD:
  case JALR:
  case SYSTEM:
  case FENCE:
    return InstructionFormat::I;

  case STORE:
    return InstructionFormat::S;

  case BRANCH:
    return InstructionFormat::B;

  case LUI:
  case AUIPC:
    return InstructionFormat::U;

  case JAL:
    return InstructionFormat::J;
  }
}

i32 get_immediate(InstructionFormat format, u32 instr_word) {

  i32 imm = 0;

  switch (format) {
  default:
    printf("Unknown format\n");
    exit(1);

  case InstructionFormat::R:
    break;

  case InstructionFormat::I:
    imm = sign_extend(extract_bits(instr_word, 20, 31), 12);
    break;

  case InstructionFormat::S:
    imm = sign_extend((extract_bits(instr_word, 25, 31) << 5) ||
                          (extract_bits(instr_word, 7, 11)),
                      12);
    break;

  case InstructionFormat::B:
    imm = sign_extend((extract_bits(instr_word, 31, 31) << 12) |
                          (extract_bits(instr_word, 7, 7) << 11) |
                          (extract_bits(instr_word, 25, 30) << 5) |
                          (extract_bits(instr_word, 8, 11) << 1),
                      13);
    break;

  case InstructionFormat::U:
    imm = static_cast<i32>(extract_bits(instr_word, 12, 31));
    break;

  case InstructionFormat::J:
    imm = sign_extend(
        (extract_bits(instr_word, 31, 31) << 20) |     // imm[20] (sign bit)
            (extract_bits(instr_word, 21, 30) << 1) |  // imm[10:1]
            (extract_bits(instr_word, 20, 20) << 11) | // imm[11]
            (extract_bits(instr_word, 12, 19) << 12),  // imm[19:12]
        21);
    break;
  }

  return imm;
}

void Instruction::decode() {

  opc = extract_bits(instr_word, 0, 6);

  rd = extract_bits(instr_word, 7, 11);
  rs1 = extract_bits(instr_word, 15, 19);
  rs2 = extract_bits(instr_word, 20, 24);

  funct3 = extract_bits(instr_word, 12, 14);
  funct7 = extract_bits(instr_word, 25, 31);

  format = get_format(opc);

  imm = get_immediate(format, instr_word);
}
