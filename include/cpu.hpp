#pragma once

#include <iostream>

#include "defs.hpp"
#include "csr.hpp"
#include "decode.hpp"

#define MEM_SIZE 0x10000

class Bus{
public:
    Bus(){
        mem = new u8[MEM_SIZE]{0};
    }

    ~Bus(){ delete[] mem; }

    u32 read(u32 addr, u32 size) {
        if (addr + size > MEM_SIZE) {
            // simple out-of-bounds check
            std::cerr << "Bus read out-of-bounds: " << std::hex << addr << "\n";
            return 0;
        }

        u32 value = 0;
        for (u32 i = 0; i < size; ++i) {
            value |= mem[addr + i] << (8 * i); // little-endian
        }
        return value;
    }


    void write(u32 addr, u32 size, u32 value) {
        if (addr + size > MEM_SIZE) {
            std::cerr << "Bus write out-of-bounds: " << std::hex << addr << "\n";
            return;
        }

        for (u32 i = 0; i < size; ++i) {
            mem[addr + i] = (value >> (8 * i)) & 0xFF; // little-endian
        }
    }

    void loadRom(u8* rom, size_t& size){
        for(int i=0;i<size;i++){
            mem[i] = rom[i];
        }
    }

private:
    u8* mem = nullptr;
};




enum class PrivilegeLevel : u32{
    User        = 0x00,
    Supervisor  = 0x01,
    Reserved    = 0x02,
    Machine     = 0x03
};

class CPUState{
public:
private:

    PrivilegeLevel level = PrivilegeLevel::Machine;

    bool sv32_enabled = false;
    bool stepping = false;
    bool debugging = false;


};

class RegFile{

public:
    RegFile(){}

    u32 read_reg(u32 idx) const { 
        return idx == 0 ? 0 : regfile[idx]; 
    }
    
    void write_reg(u32 idx, u32 value) { 
        if (idx != 0) regfile[idx] = value; 
    }

    u32 read_freg(u32 idx) const { 
        return fregfile[idx]; 
    }
    
    void write_freg(u32 idx, f32 value) { 
        fregfile[idx] = value; 
    }

    void reset(){
        for(int i=0;i<32;i++) regfile[i] = 0;
        for(int i=0;i<32;i++) fregfile[i] = 0;
    }

    void print(){
        printf("Register File:\n");
        printf("    x0:     %08X, x1/ra:  %08X, x2/sp:  %08X, x3/gp:  %08X\n",
        read_reg(0), read_reg(1), read_reg(2), read_reg(3));

        printf("    x4/tp:  %08X, x5/t0:  %08X, x6/t1:  %08X, x7/t2:  %08X\n",
            read_reg(4), read_reg(5), read_reg(6), read_reg(7));

        printf("    x8/s0:  %08X, x9/s1:  %08X, x10/a0: %08X, x11/a1: %08X\n",
            read_reg(8), read_reg(9), read_reg(10), read_reg(11));

        printf("    x12/a2: %08X, x13/a3: %08X, x14/a4: %08X, x15/a5: %08X\n",
            read_reg(12), read_reg(13), read_reg(14), read_reg(15));

        printf("    x16/a6: %08X, x17/a7: %08X, x18/s2: %08X, x19/s3: %08X\n",
            read_reg(16), read_reg(17), read_reg(18), read_reg(19));

        printf("    x20/s4: %08X, x21/s5: %08X, x22/s6: %08X, x23/s7: %08X\n",
            read_reg(20), read_reg(21), read_reg(22), read_reg(23));

        printf("    x24/s8: %08X, x25/s9: %08X,    s10: %08X,    s11: %08X\n",
            read_reg(24), read_reg(25), read_reg(26), read_reg(27));

        printf("    x28/t3: %08X, x29/t4: %08X, x30/t5: %08X, x31/t6: %08X\n",
            read_reg(28), read_reg(29), read_reg(30), read_reg(31));
    }
    

private:
    u32  regfile[32];
    u32 fregfile[32];

};


struct IF_ID{
    Instruction instr_cache;
};

struct ID_EX{
    u32 src1_value;
    u32 src2_value;
};  

struct EX_MEM{
    u32 alu_result;
    u32 link_value;
    bool branch;
};

struct MEM_WB{
    u32 mem_result;
};

// the package which contains every subpart
class RV32{
public:
    RV32() {}

    void attach_bus(Bus* bus) { this->bus = bus; }

    void reset(){
        pc = 0;
        rf.reset();
    }
    
    u32 load(u32 addr, u32 size) { return bus->read(addr, size); }
    void store(u32 addr, u32 size, u32 value) { bus->write(addr, size, value); }

    void step(){
        IF();
        ID();
        EX();
        MEM();
        WB();
    }

    void debug();


    // All architectual state: mode, pending_ex, etc.
    CPUState state;
    
    // Integer + Float register
    RegFile rf;

    // Program counter
    u32 pc;

    // machine level CSRs
    CSRUnit csr;

    
    Bus* bus = nullptr;
    
    void IF();
    
    Instruction instr_cache;
    
    void ID();

    u32 src1_value;
    u32 src2_value;
    
    void EX();
    
    u32 alu_result;
    u32 link_value;
    bool branch;
    bool jump;
    
    void MEM();

    u32 mem_result;

    void WB();
    
};