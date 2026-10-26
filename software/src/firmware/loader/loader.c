#include "loader.h"
#include "kstdio.h"
#include "arch.h"
#include "memory_map.h"
#include "ram_layout.h"
#include "panic.h"
#include "kstring.h"


__attribute__((noreturn))
void enter_kernel(uint32_t entry, uint32_t hartid, uint32_t bootinfo){

    csr_write(mepc, entry); // jump target

    register uint32_t r_a0 asm("a0") = hartid;
    register uint32_t r_a1 asm("a1") = bootinfo;

    asm volatile(
        "fence.i\n"
        "mret\n"
        :: "r"(r_a0), "r"(r_a1)
    );
    
    __builtin_unreachable();
}

void boot_kernel() {

    kprintf("Boot sequence...\n");
    kprintf("looking for kernel image at 0x%08X\n", HD_START);

    uint32_t magic = read_word(HD_START);
    kprintf("Magic: %08X\n", magic);
    
    if(magic != 0x534F5652){
        kprintf("Invalid Magic Number. Abort\n");
        return;
    } 
    
    uint32_t size  = read_word(HD_START + 4);
    kprintf("Kernel image size: %d Bytes\n", size);

    if(size == 0 || size > RAM_KERN_SIZE){
        kprintf("Kernel larger then RAM Section\n");
        return;
    }

    kprintf("Loading Kernel Image (%d Bytes)...", size);
    memcpy((void*)RAM_KERN_START, (void*)(HD_START + 8), size);   
    kprintf("Done\n");

    // kernel image is now loaded at RAM_KERN_START entry should be at RAM_KERN_START
    kprintf("Starting up\n");
    

    // set MPP
    uint32_t s = csr_read(mstatus);
    s &= ~(3u << 11);   // del MPP
    s |=  (3u << 11);   // MPP = M
    s &= ~(1u << 7);    // MPIE = 0 after mret MIE = 0
    csr_write(mstatus, s);
    csr_write(mie, 0);

    uint32_t hartid = csr_read(mhartid);

    enter_kernel(RAM_KERN_START, hartid, 0);

}   