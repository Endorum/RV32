#include "mem.h"

uint8_t read_byte(uint32_t addr){
    return *(volatile uint8_t*)(uintptr_t) addr;
}

uint32_t read_word(uint32_t addr){
    return  (uint32_t)read_byte(addr)
          | (uint32_t)read_byte(addr + 1) << 8
          | (uint32_t)read_byte(addr + 2) << 16
          | (uint32_t)read_byte(addr + 3) << 24;
}


void write_byte(uint32_t addr, uint8_t value){
    *(volatile uint8_t*)(uintptr_t) addr = value;
}

void write_word(uint32_t addr, uint32_t word){
    *(volatile uint32_t*)(uintptr_t) addr = word;    
}