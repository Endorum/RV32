
#include "../../driver/uart/include/uart.h"

#include "../include/panic.h"
#include "../include/arch.h"
#include "../../arch/rv32/include/csr.h"
#include "../../klib/include/kstdio.h"

#include "stdint.h"

#include "arch.h"



void panic(){
  kprintf("PANIC...\n");
  kprintf("Kernel Dump:\n");
  dump();
  while(1);
}