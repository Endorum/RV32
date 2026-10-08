// Boot code, asm stub to set the machine state in such a way that 
// a valid C envirtoment is created
// belongs in own seciont .text.init





.section .text.init
.global _start
_start:

  // setup trap handler to catch exceptions and interrupts
  csrw  mie, zero      // mie = 0 -> no interrupts allowed
  la    t0, trap_handler  
  csrw  mtvec, t0

  // setup stack
  la    sp, __stack_top 

  // setup global pointer
.option push
.option norelax
  la gp, __global_pointer$
.option pop

  
  // copy ROMs data to RAM
  la a0, __data_start
  la a1, __data_lma
  la a2, __data_end
  sub a2, a2, a0
  call memcpy

  // zero .bss
  la a0, __bss_start
  li a1, 0
  la a2, __bss_end
  sub a2, a2, a0
  call memset


  // clear argc and argv for now
  li a0, 0
  li a1, 0
  call main

halt:
  j halt







