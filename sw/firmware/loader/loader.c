#include "loader.h"
#include "kstdio.h"
#include "arch.h"
#include "../../global/memory_map.h"
#include "panic.h"
#include "kstring.h"

void boot_kernel() {
    kprintf("Boot sequence...\n");
    
    uint32_t magic = read_word(HD_START);
    kprintf("Magic: %08X\n", magic);
    
    if(magic != 0x534F5652){
        kprintf("Invalid Magic Number. Abort\n");
        return;
    } 
    
    uint32_t size  = read_word(HD_START + 4);
    kprintf("Kernel image size: %d Bytes\n", size);

    if(size == 0 || size >= RAM_KERN_SIZE){
        kprintf("Kernel larger then RAM Section\n");
        return;
    }

    kprintf("Loading Kernel Image...");
    memcpy((void*)RAM_KERN_START, (void*)(HD_START + 8), size);   
    kprintf("Done\n");

    // kernel image is now loaded at RAM_KERN_START entry should be at RAM_KERN_START
    kprintf("Starting up\n");
    // set mepc
    csr_write(mepc, RAM_KERN_START);

    // set MPP
    uint32_t s = csr_read(mstatus);
    s &= ~(3u << 11);   // del MPP
    s |=  (3u << 11);   // MPP = M
    csr_write(mstatus, s);

    // set a0 = hart id = 0, a1 = boot info ptr = 0
    WRITE_REG(a0, 0);
    WRITE_REG(a1, 0);

    // mret
    asm volatile("mret"); // should jump to mepc = RAM_KERN_START

}   