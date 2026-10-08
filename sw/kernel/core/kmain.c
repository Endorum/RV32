
#include "stdint.h"
#include "kstdio.h"


void kmain(uint32_t hartid, uint32_t bootinfo){
    kprintf("Kernel start, hartid %d, bootinfo %08X\n", hartid, bootinfo);


    


    kprintf("returning from kernel\n");
    return;
}