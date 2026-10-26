#include "memory_map.h"

#include "uart.h"
#include "clint.h"

#include "kstdio.h"
#include "shell.h"

#include "stddef.h"

int main(int argc, char** argv){

    // clearning screen
    // kprintf("\033[2J");
    kprintf("RUNNING FIRMWARE\n");

    uart_init();
    clint_init();

    // startup firmware shell
    start_shell(NULL);


    kprintf("HALTING EXECUTION\n");
    // asm volatile("ebreak");

    return 0;
}