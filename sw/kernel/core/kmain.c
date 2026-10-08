
#include "stdint.h"
#include "kstdio.h"
#include "kmalloc.h"
#include "page.h"

typedef struct Obj{
    int a;
    int b;
}Obj_t;

void kmain(uint32_t hartid, uint32_t bootinfo){
    kprintf("Kernel start, hartid %d, bootinfo %08X\n", hartid, bootinfo);

    
    kmalloc_init();
    
    //page_init();
    
    void* a = kmalloc(23);
    void* b = kmalloc(45);
    void* c = kmalloc(187);
    
    kmalloc_dump();

    kfree(a);
    kfree(b);
    kfree(c);

    kmalloc_dump();
    

    kprintf("returning from kernel\n");
    return;
}