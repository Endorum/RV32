#pragma once

#include "defs.hpp"

class CSRUnit{
public:
    CSRUnit(){}

    u32 read(CSR_ADDR csr_addr){
        return csr_regs[static_cast<u32>(csr_addr)];
    }

    void write(CSR_ADDR csr_addr, u32 value){
        csr_regs[static_cast<u32>(csr_addr)] = value;
    }

private:
    u32 csr_regs[0x1000] = {0};

};