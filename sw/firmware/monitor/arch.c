#include "arch.h"
#include "../../arch/rv32/include/csr.h"
#include "stdint.h"
#include "kstdio.h"

uint32_t get_reg(uint8_t idx){
  
  uint32_t value;

  switch(idx){
    default:    return 0;
    case zero:  return 0;
    case ra:    READ_REG(ra); break;
    case sp:    READ_REG(sp); break;
    case gp:    READ_REG(gp); break;
    case tp:    READ_REG(tp); break;
    case t0:    READ_REG(t0); break;
    case t1:    READ_REG(t1); break;
    case t2:    READ_REG(t2); break;
    case s0:    READ_REG(s0); break;
    case s1:    READ_REG(s1); break;
    case a0:    READ_REG(a0); break;
    case a1:    READ_REG(a1); break;
    case a2:    READ_REG(a2); break;
    case a3:    READ_REG(a3); break;
    case a4:    READ_REG(a4); break;
    case a5:    READ_REG(a5); break;
    case a6:    READ_REG(a6); break;
    case a7:    READ_REG(a7); break;
    case s2:    READ_REG(s2); break;
    case s3:    READ_REG(s3); break;
    case s4:    READ_REG(s4); break;
    case s5:    READ_REG(s5); break;
    case s6:    READ_REG(s6); break;
    case s7:    READ_REG(s7); break;
    case s8:    READ_REG(s8); break;
    case s9:    READ_REG(s9); break;
    case s10:   READ_REG(s10); break;
    case s11:   READ_REG(s11); break;
    case t3:    READ_REG(t3); break;
    case t4:    READ_REG(t4); break;
    case t5:    READ_REG(t5); break;
    case t6:    READ_REG(t6); break;

  }

  return value;
}

void set_reg(uint8_t idx, uint32_t value) {
    switch(idx){
        default:    break;
        case zero:  break; // do nothing
        case ra:    WRITE_REG(ra, value); break;
        case sp:    write_sp(value);      break;
        case gp:    WRITE_REG(gp, value); break;
        case tp:    WRITE_REG(tp, value); break;
        case t0:    WRITE_REG(t0, value); break;
        case t1:    WRITE_REG(t1, value); break;
        case t2:    WRITE_REG(t2, value); break;
        //case s0:    WRITE_REG(s0, value); break;
        case s0:    asm volatile("mv s0, %0" :: "r"(value));
        case s1:    WRITE_REG(s1, value); break;
        case a0:    WRITE_REG(a0, value); break;
        case a1:    WRITE_REG(a1, value); break;
        case a2:    WRITE_REG(a2, value); break;
        case a3:    WRITE_REG(a3, value); break;
        case a4:    WRITE_REG(a4, value); break;
        case a5:    WRITE_REG(a5, value); break;
        case a6:    WRITE_REG(a6, value); break;
        case a7:    WRITE_REG(a7, value); break;
        case s2:    WRITE_REG(s2, value); break;
        case s3:    WRITE_REG(s3, value); break;
        case s4:    WRITE_REG(s4, value); break;
        case s5:    WRITE_REG(s5, value); break;
        case s6:    WRITE_REG(s6, value); break;
        case s7:    WRITE_REG(s7, value); break;
        case s8:    WRITE_REG(s8, value); break;
        case s9:    WRITE_REG(s9, value); break;
        case s10:   WRITE_REG(s10, value); break;
        case s11:   WRITE_REG(s11, value); break;
        case t3:    WRITE_REG(t3, value); break;
        case t4:    WRITE_REG(t4, value); break;
        case t5:    WRITE_REG(t5, value); break;
        case t6:    WRITE_REG(t6, value); break;

    }
}

uint32_t read_pc(void) {
    uint32_t v;
    asm volatile("auipc %0, 0" : "=r"(v));   // pc + (0 << 12) = Adresse dieser Instruktion
    return v;
}

uint32_t read_sp(void) {
    uint32_t v;
    asm volatile("mv %0, sp" : "=r"(v));
    return v;
}

void write_sp(uint32_t value){
    asm volatile(
        "mv sp, %0\n" :: "r"(sp)
    );
}

void write_pc(uint32_t value){
    asm volatile(
        "jr %0\n"
        :: "r"(value)
    );
}

__attribute__((noreturn))
void jump_to(uint32_t pc, uint32_t sp) {
    asm volatile(
        "mv sp, %0\n"
        "jr %1\n"
        :: "r"(sp), "r"(pc)
    );
    __builtin_unreachable();
}

// register dump
void dump(){
  // print all registers
  kprintf("Register file:\n");
  for(int i=0;i<32;i++){
    kprintf("\tx%d \t = \t 0x%x\n",i,get_reg(i));
  }

  // print some of the csrs
  kprintf("\nCommon CSRs:\n");
  kprintf("\tmstatus: 0x%x\n",  csr_read(mstatus));
  kprintf("\tmtvec: 0x%x\n",    csr_read(mtvec));
  kprintf("\tmepc: 0x%x\n",     csr_read(mepc));
  kprintf("\tmcause: 0x%x\n",   csr_read(mcause));
  kprintf("\tmtval: 0x%x\n",    csr_read(mtval));
  kprintf("\tmscratch: 0x%x\n", csr_read(mscratch));

  kprintf("Current PC: %08X\n", read_pc());
}


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