#include "stdint.h"
#include "trap.h"

/*
 * Monitor shell — inspired by Wozmon (MOS 6502).
 *
 * A single pointer marks the current location. All numbers are hex
 * (optional 0x prefix). <range> is <start> <end>.
 * The prompt shows the current pointer: "80000000> "
 *
 * Memory
 *   show [<start>|<range>]   hex + ascii dump (default: BLOCK bytes at pointer)
 *                            does not move the pointer
 *   <arrow down>             show current block and move pointer behind it
 *   <arrow up>               show previous block and move pointer infront of it
 *   dis  [<start>|<range>]   disassemble, does not move the pointer
 *
 *   write <value>            writes the value to current pointer, pointer moves behind last byte
 *   write <addr> <value>     writes the value to address, does NOT move the pointer
 *   write "text"             write raw chars (no '\0'), escapes: \n \t \0 \\ \"
 *                            pointer moves behind last byte
 *   print                    print string at pointer until '\0'
 *   input                    read a line, write it at pointer + '\0'
 *                            pointer stays (so 'print' echoes it back)
 *
 *   copy <range> <dest>      copy bytes, overlap-safe (memmove semantics)
 *
 * Pointer
 *   goto <addr>              set pointer to addr
 *   goto *                   set pointer to the 32-bit little-endian value at
 *                            pointer, e.g. 12 34 56 78 -> 0x78563412      
 *   goto -                   go back to previous pointer
 *                            
 *
 * Execution
 *   exec [<addr>]            execute at addr (default: pointer)
 *                            'ebreak' returns control to the shell
 *
 * Registers
 *   regs                     show x0-x31, pc and common CSRs
 *   set <reg> <value>        reg: x0-x31, ABI name (sp, a0, ...) or pc
 *                            writes to x0 are rejected
 *
 * Misc
 *   block <value>            sets the block size
 *   help                     list commands
 *   exit                     leave the shell
 */

void set_pointer(uint32_t p);

void start_shell(trap_frame_t* tf);
int shell(trap_frame_t* tf);