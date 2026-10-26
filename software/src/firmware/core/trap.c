#include <stdint.h>

#include "uart.h"
#include "kstdio.h"
#include "csr.h"

#include "arch.h"
#include "panic.h"
#include "trap.h"

#include "shell.h"

#include "../../../platform/devices/clint.h"


void trap(trap_frame_t* tf){

    uint32_t cause = csr_read(mcause);
    uint32_t tval = csr_read(mtval);

    if((cause & 0x80000000) == 0)         exc_handler(tf, cause, tval);
    else if(cause == MACH_SOFTWARE_INT)   msi_handler(tf);
    else if(cause == MACH_TIMER_INT)      mti_handler(tf);

    else{
        kprintf("Unknown cause: %x\n", cause);
        panic(tf, cause, tval);
    }

}



void exc_handler(trap_frame_t* tf, uint32_t cause, uint32_t tval){
    
    switch(cause){

        case ECALL_FROM_U_MODE:
        case ECALL_FROM_S_MODE:
        case ECALL_FROM_M_MODE:
            handle_ecall(tf, cause);
            tf->mepc += 4;
            return;

        case BREAKPOINT:
            kprintf("BREAKPOINT, at %08X\n", tf->mepc);
            // call shell(tf) for debugging
            start_shell(tf);
            tf->mepc += 4;
            return;
        

        case INSTR_ADDR_MISALIGNED:
            kprintf("INSTR_ADDR_MISALIGNED, tval: %08X\n", tval);
            break;
        case INSTR_ACCESS_FAULT:
            kprintf("INSTR_ACCESS_FAULT, tval: %08X\n", tval);
            break;
        case ILLEGAL_INSTRUCTION:
            kprintf("ILLEGAL_INSTRUCTION, tval: %08X\n", tval);
            break;
        case LOAD_ADDR_MISALIGNED:
            kprintf("LOAD_ADDR_MISALIGNED, tval: %08X\n", tval);
            break;
        case LOAD_ACCESS_FAULT:
            kprintf("LOAD_ACCESS_FAULT, tval: %08X\n", tval);
            break;
        case STORE_ADDR_MISALIGNED:
            kprintf("STORE_ADDR_MISALIGNED, tval: %08X\n", tval);
            break;
        case STORE_ACCESS_FAULT:
            kprintf("STORE_ACCESS_FAULT, tval: %08X\n", tval);
            break;
        case INSTR_PAGE_FAULT:
            kprintf("INSTR_PAGE_FAULT, tval: %08X\n", tval);
            break;
        case LOAD_PAGE_FAULT:
            kprintf("LOAD_PAGE_FAULT, tval: %08X\n", tval);
            break;
        case STORE_PAGE_FAULT:
            kprintf("STORE_PAGE_FAULT, tval: %08X\n", tval);
            break;
        case DOUBLE_TRAP:
            kprintf("DOUBLE_TRAP, tval: %08X\n", tval);
            break;
        case SOFTWARE_CHECK:
            kprintf("SOFTWARE_CHECK, tval: %08X\n", tval);
            break;
        case HARDWARE_ERROR:
            kprintf("HARDWARE_ERROR, tval: %08X\n", tval);
            break;


        default: 
            kprintf("Unknown exception, mcause: %08X\n", cause);
            break;
    }

    // panic for every exception that is not ebreak or ecall
    panic(tf, cause, tval);

}

void handle_ecall(trap_frame_t *tf, uint32_t cause) {
        kprintf("ECALL, cause: %x\n", cause);

        switch(cause){


            case ECALL_FROM_M_MODE: 
                    // services e.g. putchar or something
                    kprintf("Firmware service\n");
                    break;

            case ECALL_FROM_S_MODE: break;
            case ECALL_FROM_U_MODE: break;
        }

        return;
}

void msi_handler(trap_frame_t* tf){
    kprintf("Machine software interrupt\n");
    
    write_word(CLINT_START + MSIP_ADDR, 0); // msip = 0 -> source off
}

void mti_handler(trap_frame_t* tf){
    kprintf("Machine Timer interrupt\n");

    // disable mti
    write_word(CLINT_START + MTIMECMP_ADDR + 4, 0xFFFFFFFF); // write high first
    write_word(CLINT_START + MTIMECMP_ADDR,     0xFFFFFFFF); // low
}