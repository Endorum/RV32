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
        if(idx != 0) { 
            regfile[idx] = value; 
        } 
    }

    u32 read_freg(u32 idx) const { 
        return fregfile[idx]; 
    }
    
    void write_freg(u32 idx, f32 value) { 
        fregfile[idx] = value; 
    }
    

private:
    u32  regfile[32];
    u32 fregfile[32];

};


// the package which contains every subpart
class RV32{
public:
    RV32() {}

    void attach_bus(Bus* bus) { this->bus = bus; }

    CPUState get_state(){ return state; }
    RegFile get_regfile(){ return rf; }
    u32 get_pc(){ return pc; }
    CSRUnit get_csr(){ return csr; }
    Bus* get_bus(){ return bus; }  
    
    u32 load(u32 addr, u32 size) { return bus->read(addr, size); }
    void store(u32 addr, u32 size, u32 value) { bus->write(addr, size, value); }

    void step(){
        IF();
        ID();
        EX();
        MEM();
        WB();
    }

private:
    // All architectual state: mode, pending_ex, etc.
    CPUState state;
    
    // Integer + Float register
    RegFile rf;

    // Program counter
    u32 pc;

    // machine level CSRs
    CSRUnit csr;

    
    Bus* bus = nullptr;
    
    Instruction instr_cache;
    void IF();
    
    u32 src1_value, src2_value;
    void ID();
    
    u32 alu_result;
    u32 link_value;
    void EX();
    
    void MEM();
    void WB();
    
};