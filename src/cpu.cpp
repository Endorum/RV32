#include "cpu.hpp"

#include "decode.hpp"
#include "execute.hpp"

void RV32::IF() {
    u32 instr_addr = pc;

    u32 instr = bus->read(instr_addr, WORD);

    instr_cache.set_instr_word(instr);
    instr_cache.set_instr_addr(pc);
    
    pc += 4;
}

void RV32::ID() {
    instr_cache.decode();

    // preload from the registers
    src1_value = get_regfile().read_reg(instr_cache.rs1); 
    src2_value = get_regfile().read_reg(instr_cache.rs2);

}

void RV32::EX() {

    

    switch(instr_cache.format){
        default:break;

        case InstructionFormat::R:
            alu_result = alu_r_type(src1_value, src2_value, &instr_cache);
            break;

        case InstructionFormat::I: 
            alu_result = alu_i_type(src1_value, &instr_cache);
            link_value = pc + 4;
            break;
        case InstructionFormat::S: 

            break;
        case InstructionFormat::B: 

            break;
        case InstructionFormat::U: 

            break;
        case InstructionFormat::J: 

            break;

    }

}

void RV32::MEM() {

}

void RV32::WB() {

}
