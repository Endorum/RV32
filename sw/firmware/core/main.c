#include "../../global/memory_map.h"

#include "../../driver/uart/include/uart.h"

#include "kstdio.h"
#include "shell.h"

int main(int argc, char** argv){

    uart_init();

    // repl shell
    set_pointer(RAM_START);

    

    while(1){
        
        int code = shell();

        if(code > 0) 
                break;

    }



    kprintf("HALTING EXECUTION\n");
    asm volatile("ebreak");

    return 0;
}