
#include "uart.h"

#include "panic.h"
#include "arch.h"
#include "csr.h"
#include "kstdio.h"

#include "stdint.h"

#include "arch.h"



void panic(){
  kprintf("PANIC...\n");
  kprintf("Kernel Dump:\n");
  dump();
  while(1);
}