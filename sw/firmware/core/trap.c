#include <stdint.h>

#include "../../driver/uart/include/uart.h"
#include "../../klib/include/kstdio.h"
#include "../../arch/rv32/include/csr.h"

#include "../include/arch.h"
#include "../include/panic.h"
#include "../include/trap.h"


void trap(){
  uint32_t cause = csr_read(mcause);
  uint32_t tval = csr_read(mtval);

  if((cause & 0x80000000) == 0){
    exc_handler(cause, tval);
  }
  else if(cause == MACH_SOFTWARE_INT){
    msi_handler();
  }
  else if(cause == MACH_TIMER_INT){
    mti_handler();
  }
  else{
    kprintf("Unknown cause: %x\n", cause);
    panic();
  }

}



void exc_handler(uint32_t cause, uint32_t word){
  
  switch(cause){
    default: 
      kprintf("Unknown exception, mcause: %x\n", cause);
      panic();
      break; 

    case INSTR_ADDR_MISALIGNED:
      kprintf("INSTR_ADDR_MISALIGNED, word: %x\n", word);
      break;
    case INSTR_ACCESS_FAULT:
      kprintf("INSTR_ACCESS_FAULT, word: %x\n", word);
      break;
    case ILLEGAL_INSTRUCTION:
      kprintf("ILLEGAL_INSTRUCTION, word: %x\n", word);
      break;
    case BREAKPOINT:
      kprintf("BREAKPOINT, word: %x\n", word);
      csr_write(mepc, csr_read(mepc) + 4);
      break;
    case LOAD_ADDR_MISALIGNED:
      kprintf("LOAD_ADDR_MISALIGNED, word: %x\n", word);
      break;
    case LOAD_ACCESS_FAULT:
      kprintf("LOAD_ACCESS_FAULT, word: %x\n", word);
      break;
    case STORE_ADDR_MISALIGNED:
      kprintf("STORE_ADDR_MISALIGNED, word: %x\n", word);
      break;
    case STORE_ACCESS_FAULT:
      kprintf("STORE_ACCESS_FAULT, word: %x\n", word);
      break;
    case ECALL_FROM_U_MODE:
      kprintf("ECALL_FROM_U_MODE, word: %x\n", word);
      csr_write(mepc, csr_read(mepc) + 4);
      break;
    case ECALL_FROM_S_MODE:
      kprintf("ECALL_FROM_S_MODE\n");
      csr_write(mepc, csr_read(mepc) + 4);
      break;
    case ECALL_FROM_M_MODE:
      kprintf("ECALL_FROM_M_MODE\n");
      csr_write(mepc, csr_read(mepc) + 4);
      break;
    case INSTR_PAGE_FAULT:
      kprintf("INSTR_PAGE_FAULT, word: %x\n", word);
      break;
    case LOAD_PAGE_FAULT:
      kprintf("LOAD_PAGE_FAULT, word: %x\n", word);
      break;
    case STORE_PAGE_FAULT:
      kprintf("STORE_PAGE_FAULT, word: %x\n", word);
      break;
    case DOUBLE_TRAP:
      kprintf("DOUBLE_TRAP, word: %x\n", word);
      break;
    case SOFTWARE_CHECK:
      kprintf("SOFTWARE_CHECK, word: %x\n", word);
      break;
    case HARDWARE_ERROR:
      kprintf("HARDWARE_ERROR, word: %x\n", word);
      break;
  }

  // panic for all exceptions on default
  panic();
}

void msi_handler(){
  kprintf("Machine software interrupt\n");

  return;
}

void mti_handler(){
  kprintf("Machine Timer interrupt\n");

  return;
}