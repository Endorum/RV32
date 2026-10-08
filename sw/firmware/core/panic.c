
#include "uart.h"

#include "panic.h"
#include "arch.h"
#include "csr.h"
#include "kstdio.h"

#include "stdint.h"

#include "arch.h"



void panic(trap_frame_t* tf, uint32_t cause, uint32_t tval){
  kprintf("PANIC...\n");
  kprintf("Kernel Dump:\n");
  kprintf("Trap Frame:\n");
  
  kprintf("Registers:\n");
  for(int i=0;i<32;i++){
    kprintf("\tx%d \t = \t 0x%x\n",i,tf->x[i]);
  }

  kprintf("mepc:    %08X\n", tf->mepc);
  kprintf("mstatus: %08X\n",tf->mstatus);
  kprintf("cause:   %08X\n",cause);
  kprintf("tval:    %08X\n",tval);


  jump_to(ROM_START, RAM_START + RAM_SIZE);
}