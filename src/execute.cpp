
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

    u32 funct7 = (imm >> 5) & 0x7F;

    switch(instr->opc){
        default:
            break;


        case ALU_I: // Arithmetic Imm

            switch(instr->funct3){
                default:break;

                case 0x0: instr->mnemonic = "addi";  return src1 + imm;
                case 0x1: instr->mnemonic = "slli";  return src1 << shmt;
                case 0x2: instr->mnemonic = "slti";  return (i32(src1) < imm) ? 1 : 0;
                case 0x3: instr->mnemonic = "sltiu"; return (src1 < uimm) ? 1 : 0;
                case 0x4: instr->mnemonic = "xori";  return src1 ^ imm;

                case 0x5:
                    if (funct7 == 0x20) { // 0b0100000
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


        case LOAD:
            switch(instr->funct3) {
                case 0x0: instr->mnemonic = "lb";  return s_src1 + imm;
                case 0x1: instr->mnemonic = "lh";  return s_src1 + imm;
                case 0x2: instr->mnemonic = "lw";  return s_src1 + imm;
                case 0x4: instr->mnemonic = "lbu"; return src1   + imm;
                case 0x5: instr->mnemonic = "lhu"; return src1   + imm;


                default:
                    printf("Invalid load funct3\n"); exit(1);
            }
            

        case JALR: instr->mnemonic = "jalr"; return (s_src1 + imm) & ~1; // jalr target

        case ECALL: // ecall or ebreak
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

    return 0;
}

bool evaluate_branch(u32 src1, u32 src2, Instruction *instr) {

    i32 sA = static_cast<i32>(src1);
    i32 sB = static_cast<i32>(src2);

    u32 uA = src1;
    u32 uB = src2;
    
    switch(instr->funct3){
        default:break;

        case 0x0: instr->mnemonic = "beq";  return sA == sB;
        case 0x1: instr->mnemonic = "bne";  return sA != sB;
        case 0x4: instr->mnemonic = "blt";  return sA <  sB;
        case 0x5: instr->mnemonic = "bge";  return sA >= sB;
        case 0x6: instr->mnemonic = "bltu"; return uA <  uB;
        case 0x7: instr->mnemonic = "bgeu"; return uA >= uB;

    }


    printf("ERROR: Unknown funct3 value in evaluate_branch: %02X\n",instr->funct3);
    exit(1);

    return false;
}

u32 compute_u_type(Instruction *instr) {

    switch(instr->opc){
        default:break;

        case LUI: instr->mnemonic = "lui";   return instr->imm << 12; // Load Upper imm
        case AUIPC: instr->mnemonic = "auipc"; return instr->instr_addr + (instr->imm << 12); // add upper imm to pc

    }

    printf("ERROR: Unknown opcode value in compute_u_type: %02X\n",instr->opc);
    exit(1);

    return 0;
}
