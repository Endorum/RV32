
#include "devices/screenInfo.h"
#include "fat12.h"
#include "hd.h"
#include "memory_map.h"
#include "mm_layout.h"
#include "shell.h"
#include "stdint.h"
#include "kstdio.h"
#include "kmalloc.h"
#include "page.h"
#include "kstring.h"

#include "../drivers/screen/screen.h"
#include "csr.h"

#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10
#define PTE_G 0x20
#define PTE_A 0x40
#define PTE_D 0x80

void map_superpage(uint32_t* root, uint32_t va, uint32_t pa, uint32_t flags){
    root[va >> 22] = ((pa >> 12) << 10) | flags;
}

void init_vmemory(){
    
    kprintf("Initializing virtual memory...");
    
    // 4KiB
    uint32_t* root = alloc_page();
    memset(root, 0, PAGE_SIZE);
    // every 4byte word is a pte

    // entries for identity mapped pages
    // identity mapping the whole of RAM
    for(uint32_t a = RAM_START; a < RAM_START + RAM_SIZE; a += 0x400000){
        map_superpage(root, a, a, PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D);
    }

    // for uart
    map_superpage(root, UART_START, UART_START, PTE_V|PTE_R|PTE_W|PTE_A|PTE_D);
    
    // for hd
    for(uint32_t a = HD_START; a < HD_START + HD_SIZE; a += 0x400000){
        map_superpage(root, a, a, PTE_V|PTE_R|PTE_W|PTE_X|PTE_A|PTE_D);
    }
    
    kprintf("Done\n");


    uint32_t satp = (1u << 31) | ((uint32_t)root >> 12);
    csr_write(satp, satp);

    asm volatile ("sfence.vma");

    kprintf("paging on\n");
    
}





void kmain(uint32_t hartid, uint32_t bootinfo){
    kprintf("Kernel start, hartid %d, bootinfo %08X\n", hartid, bootinfo);

    
    kmalloc_init();
    
    page_init();
    
    init_vmemory();
    
    fat12_mount();
        
    start_shell();

    
    
    kprintf("end of kernel, entering infinit loop\n");
    
    while(1) asm ("wfi");

    kprintf("returning from kernel\n");
    return;
}