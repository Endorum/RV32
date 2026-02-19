
#include "execute.hpp"

u32 alu_r_type(u32 src1, u32 src2, Instruction* instr) {

    i32 s_src1 = static_cast<i32>(src1);
    i32 s_src2 = static_cast<i32>(src2);

    u32 shmt = instr->rs2 & 0x1F;

    // RV32I
    if(instr->funct7 == 0x00){
        switch(instr->funct3){
            default:break;

            case 0x0: instr->mnemonic = "add";  return src1 + src2;
            case 0x1: instr->mnemonic = "sll";  return src1 << shmt;
            case 0x2: instr->mnemonic = "slt";  return (s_src1 < s_src2) ? 1 : 0;
            case 0x3: instr->mnemonic = "sltu"; return (src1 < src2) ? 1 : 0;
            case 0x4: instr->mnemonic = "xor";  return src1 ^ src2;
            case 0x5: instr->mnemonic = "srl";  return src1 >> shmt;
            case 0x6: instr->mnemonic = "or";   return src1 | src2;
            case 0x7: instr->mnemonic = "and";  return src1 & src2;

        }
    }else if(instr->funct7 == 0x20){    
        switch(instr->funct3){
            default:break;

            case 0x0: instr->mnemonic = "sub";  return src1 - src2;
            case 0x5: instr->mnemonic = "sra";  return s_src1 >> shmt;

        }
    }


    printf("ERROR: Unknown funct7 value in alu_r_type: %02X\n",instr->funct7);
    exit(1);

    return 0;
}



u32 alu_i_type(u32 src1, Instruction *instr) {

    i32 imm = instr->imm;
    u32 uimm = static_cast<u32>(instr->imm);

    i32 s_src1 = static_cast<i32>(src1);

    u32 shmt = imm & 0x1F;

    u32 funct7 = extract_bits(uimm, 5, 11);

    switch(instr->opc){
        default:break;


        case 0b0010011: // Arithmetic Imm

            switch(instr->funct3){
                default:break;

                case 0x0: instr->mnemonic = "addi";  return src1 + imm;
                case 0x1: instr->mnemonic = "slli";  return src1 << shmt;
                case 0x2: instr->mnemonic = "slti";  return (i32(src1) < imm) ? 1 : 0;
                case 0x3: instr->mnemonic = "sltiu"; return (src1 < uimm) ? 1 : 0;
                case 0x4: instr->mnemonic = "xori";  return src1 ^ imm;

                case 0x5:
                    if( (imm >> 10) & 1 ){ // check bit 10 of imm -> srai
                        instr->mnemonic = "srai";
                        return (i32(src1)) >> shmt;
                    } else {
                        instr->mnemonic = "srli";
                        return src1 >> shmt;
                    }

                case 0x6: instr->mnemonic = "ori"; return src1 | imm;
                case 0x7: instr->mnemonic = "andi"; return src1 & imm;

            }

            break; 


        case 0b0000011: return s_src1 + imm; // Load ops
        case 0b1100111: return s_src1 + imm; // jalr target
        case 0b1110011: // ecall or ebreak
            if(imm == 0x0){
                instr->mnemonic = "ecall";
            }else if(imm == 0x1){
                instr->mnemonic = "ebreak";
            }
            // nothing implemented yet
            return 0;
    }

    printf("ERROR: Unknown opcode value in alu_i_type: %02X\n",instr->opc);
    exit(1);

    return u32();
}
