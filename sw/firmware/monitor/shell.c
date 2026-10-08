#include "memory_map.h"
#include "uart.h"

#include "shell.h"
#include "stdint.h"
#include "kstdio.h"

#include "kstring.h"

#include "arch.h"

#include "loader.h"

uint32_t pointer;
uint32_t prev_pointer;

uint32_t block_size = 512; // 128 bytes




void set_pointer(uint32_t addr){
    prev_pointer = pointer;
    pointer = addr;
}

void print_help(){
    kprintf(""
    " A single pointer marks the current location. All numbers are hex\n"
    " (optional 0x prefix). <range> is <start>-<end>.\n"
    " The prompt shows the current pointer: '80000000> '\n"
    "\n"
    " Memory\n"
    "   show [<start>|<range>]   hex + ascii dump (default: BLOCK bytes at pointer)\n"
    "                            does not move the pointer\n"
    "   scroll                   show next block and move pointer behind it\n"
    "   dis  [<start>|<range>]   disassemble, does not move the pointer\n"
    "\n"
    "   write <value>            writes the value to current pointer, pointer moves behind last byte\n"
    "   write <addr> <value>     writes the value to address, pointer moves behind last byte\n"
    "   write \"text\"             write raw chars (no '\\0'), escapes: \\n \\t \\0 \\\\ \\\"\n"
    "                            pointer moves behind last byte\n"
    "   print                    print string at pointer until '\\0'\n"
    "   input                    read a line, write it at pointer + '\\0'\n"
    "                            pointer stays (so 'print' echoes it back)\n"
    "\n"
    "   copy <range> <dest>      copy bytes, overlap-safe (memmove semantics)\n"
    "\n"
    " Pointer\n"
    "   goto <addr>              set pointer to addr\n"
    "   goto *                   set pointer to the 32-bit little-endian value at\n"
    "                            pointer, e.g. 12 34 56 78 -> 0x78563412\n"
    "   goto -                   go back to previous pointer\n"
    "\n"
    " Execution\n"
    "   exec [<addr>]            execute at addr (default: pointer)\n"
    "                            'ebreak' returns control to the shell\n"
    "\n"
    " Registers\n"
    "   regs                     show x0-x31, pc and common CSRs\n"
    "   set <reg> <value>        reg: x0-x31, ABI name (sp, a0, ...) or pc\n"
    "                            writes to x0 are rejected\n"
    "\n"
    " Misc\n"
    "   help                     list commands\n"
    "   exit                     leave the shell\n");
}

char sanitize_char(const char c){
    if ( c >= '!' && c <= '~') return c;
    else return '.';
}

void print_show(uint32_t start, uint32_t end){

    uint32_t cols = 32;
    uint32_t col = 0;

    for(uint32_t off = 0; off <= end - start - 1; off++){

        
        // end row
        if(off % cols == 0 && off > 0){
            col = 0;
            kprintf("\n");
        }
        
        // start of row
        if(off % cols == 0){
            kprintf("%08X: ", start + off);
        }
        
        if(col < cols/2){
            kprintf("%02X ", read_byte(off + start));
        }else{
            kprintf("%c", sanitize_char( read_byte(off + start) ));
        }
        
        col++;
    }
    
    kprintf("\n");

}

int tokenize(char* s, char** argv, int max){
    int argc = 0;

    while(*s && argc < max){
        while(*s == ' ') s++; // skip space
        if(!*s) break;

        argv[argc++] = s;

        if(*s == '"'){
            s++;
            while(*s && *s != '"') s++; // until closing "

        }else{
            while(*s && *s != ' ') s++; // until next space
        }

        if (*s) *s++ = '\0'; // terminate token
    }

    return argc;
}

// typedef struct {
//     uint32_t x[32];
//     uint32_t pc;
// }context_t;

// context_t ctx;

// void store_context(){
//     for(int i=0;i<32;i++){
//         ctx.x[i] = get_reg(i);
//     }

//     ctx.pc = read_pc();
// }

// void load_context(){
//     for(int i=0;i<32;i++){
//         set_reg(i, ctx.x[i]);
//     }   
// }

// static uint8_t prog_stack[4096] __attribute__((aligned(16)));


uint32_t parse_argument(const char* s, int* bytes){
    
    if(!s) return 0;
    if(bytes) *bytes = 4;

    if(strcmp(s, "ROM_START") == 0)   return ROM_START;
    if(strcmp(s, "ROM_SIZE") == 0)    return ROM_SIZE;
    if(strcmp(s, "ROM_END") == 0)     return ROM_START + ROM_SIZE;

    if(strcmp(s, "HD_START") == 0)    return HD_START;
    if(strcmp(s, "HD_SIZE") == 0)     return HD_SIZE;
    if(strcmp(s, "HD_END") == 0)      return HD_START + HD_SIZE;

    if(strcmp(s, "RAM_START") == 0)   return RAM_START;
    if(strcmp(s, "RAM_SIZE") == 0)    return RAM_SIZE;
    if(strcmp(s, "RAM_END") == 0)     return RAM_START + RAM_SIZE;

    if(strcmp(s, "CLINT_START") == 0) return CLINT_START;
    if(strcmp(s, "CLINT_SIZE") == 0)  return CLINT_SIZE;
    if(strcmp(s, "CLINT_END") == 0)   return CLINT_START + CLINT_SIZE;

    if(strcmp(s, "UART_START") == 0)  return UART_START;
    if(strcmp(s, "UART_SIZE") == 0)   return UART_SIZE;
    if(strcmp(s, "UART_END") == 0)    return UART_START + UART_SIZE;
    
    if(strcmp(s, ".") == 0) return pointer;

    else if(s[0] == '-' && strlen(s) > 1){
        return pointer - parse_argument(s + 1, bytes);
    }
    else if(s[0] == '+' && strlen(s) > 1){
        return pointer + parse_argument(s + 1, bytes);
    }

    return parse_hex(s, bytes);
}

int shell(){
    // 1. print current pointer address
    kprintf("%08X:> ", pointer);

    // 2. get user input
    char input[256];
    uart_gets(input, 256);

    // 3. eval
    
    char* argv[4];
    int argc = tokenize(input, argv, 4);
    if(argc == 0) return 0;

    const char* cmd = argv[0];
    char *arg1 = argc > 1 ? argv[1] : NULL;
    char *arg2 = argc > 2 ? argv[2] : NULL;
    char *arg3 = argc > 3 ? argv[3] : NULL;

    // exit the shell
    if(strcmp(cmd, "exit") == 0) return 1;

    // dumps register contents as well as some other debug stuff
    if(strcmp(cmd, "regs") == 0){
        dump();
    }

    // help
    else if(strcmp(cmd, "help") == 0){
        print_help();
    }

    else if(strcmp(cmd, "[A") == 0){
        // history up
    }

    else if(strcmp(cmd, "[B") == 0){
        // history down
    }

    // set block size
    else if(strcmp(cmd, "block") == 0){
        if(argc == 2){
            uint32_t new_size = parse_argument(arg1, NULL);
            kprintf("block size: %d -> %d\n", block_size, new_size);
            block_size = new_size;
        }
    }

    // show at current pointer fixed length
    else if(strcmp(cmd, "show") == 0){
        if(argc == 1)      print_show(pointer, pointer+block_size); // no args
        else if(argc == 2){
            // single argument -> starting address of show
            uint32_t start = parse_argument(arg1, NULL);
            print_show(start, start+block_size);
        }
        else if(argc == 3){
            // two arguments -> start and end
            uint32_t start = parse_argument(arg1, NULL);
            uint32_t end = parse_argument(arg2, NULL);
            print_show(start, end);
        }
    }

    else if(strcmp(cmd, "n") == 0 || strcmp(cmd, "next") == 0){
        if(argc == 1){
            set_pointer(pointer + block_size);
            print_show(pointer, pointer + block_size);
        }
    }

    else if(strcmp(cmd, "p") == 0 || strcmp(cmd, "prev") == 0){
        if(argc == 1){
            set_pointer(pointer - block_size);
            print_show(pointer, pointer + block_size);
        }
    }

    else if(strcmp(cmd, "goto") == 0){
        
        if (argc != 2) {
            kprintf("usage: goto <addr> | goto * | goto -\n");
        }

        if(argc == 2){
            if(arg1[0] == '*'){
                uint32_t addr = read_word(pointer);
                set_pointer(addr);
            }
            else if(strcmp(arg1, "-") == 0){
                set_pointer(prev_pointer);
            }
            
            else{
                int digits;
                uint32_t addr = parse_argument(arg1, &digits);
                if (digits == 0) kprintf("invalid address: %s\n", arg1);
                else             set_pointer(addr);
            }
        }
    }

    else if(strcmp(cmd, "write") == 0){
        if(argc == 1){
            kprintf("usage: write <value> | write <addr> <value> | write \"string\"\n");
        }
        if(argc == 2){
            if(arg1[0] == '"'){
                arg1 += 1; // skip "
                arg1[strlen(arg1)] = '\0'; // overwrite " with 0

                kprintf("String %s\n", arg1);
                
                int i=0;
                for(;i<strlen(arg1) + 1;i++){
                    write_byte(pointer + i, arg1[i]);
                }

                set_pointer(pointer + i);

            }else{
                int bytes;
                uint32_t value = parse_argument(arg1, &bytes);
                if (bytes == 0) kprintf("invalid address: %s\n", arg1);
                if(bytes == 1){
                    write_byte(pointer, value);
                    set_pointer(pointer + 1); // move pointer behind writen word
                }else{
                    write_word(pointer, value);
                    set_pointer(pointer + 4); // move pointer behind writen word
                }
                
            }
        }
        if(argc == 3){
            uint32_t addr   = parse_argument(arg1, NULL);
            uint32_t value  = parse_argument(arg2, NULL);
            if(value <= 0xFF)
                write_byte(addr, value);
            else
                write_word(addr, value);
            
        }

    }

    else if(strcmp(cmd, "print") == 0){
        int i=0;
        while(1){
            char b = read_byte(pointer + i++);
            kprintf("%c", b);

            if(b == 0) break;
        }
        kprintf("\n");
    }

    else if(strcmp(cmd, "input") == 0){
        char buf[256];
        uart_gets(buf, 256);
        int i;
        for(i=0;i<strlen(buf);i++){
            write_byte(pointer+i, buf[i]);
        }
        write_byte(pointer+i, '\0');
    }

    else if(strcmp(cmd, "set") == 0){
        if(argc != 3){
            kprintf("Usage: set <register index> <hex value>\n");
        }
        else{
            uint32_t index = parse_argument(arg1, NULL);
            uint32_t value = parse_argument(arg2, NULL);
            set_reg(index, value);
            kprintf("Set register x%d to value %x\n", index, value);
        }
    }

    // bascially just jalr <addr>
    else if(strcmp(cmd, "exec") == 0){
        uint32_t addr = (argc == 2) ? parse_argument(arg1, NULL) : pointer;
        asm volatile("fence.i");

        ((void (*)(void))(uintptr_t)addr)(); // cast address as a function and call it
    }

    

    else if(strcmp(cmd, "copy") == 0){
        if(argc != 4){
            kprintf("usage: copy source_start source_end destination_start\n");
        } 
        else{
            uint32_t src_start = parse_argument(arg1, NULL);
            uint32_t src_end   = parse_argument(arg2, NULL);
            uint32_t dst_start = parse_argument(arg3, NULL);

            memcpy((void*)dst_start, (const void*)src_start, src_end - src_start);

            kprintf("Copied %d bytes\n", src_end - src_start);

        }
    }

    else if(strcmp(cmd, "boot") == 0){
        boot_kernel();        
    }
    
    
    return 0;
}

