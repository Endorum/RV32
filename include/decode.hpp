#pragma once

#include <string>

#include "defs.hpp"

enum class InstructionFormat{
    NONE,
    R,
    I,
    S,
    B,
    U,
    J
};


class Instruction{
public:
    Instruction(){}

    void decode();

    void set_instr_word( u32 word ){ instr_word = word; }
    void set_instr_addr( u32 addr ){ instr_addr = addr; }

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

    void print(){

        printf(
            "word: %08X\naddr: %08X\nopc: %02X\nrs2: %d,rs1: %d rd: %d\nfunct3: %02X\nfunct7: %02X\nimm: %08X\nmnemonic: %s\n",
            instr_word, 
            instr_addr, 
            opc, 
            rs2, 
            rs1,
            rd,
            funct3,
            funct7,
            imm,
            mnemonic.c_str()

        );
    }
};


u32 extract_bits(u32 value, u32 min, u32 max);