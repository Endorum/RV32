
.global _start

.section .text.init
_start:

    // saving a0 and a1
    mv t0, a0
    mv t1, a1
        
    // setup stack
    la    sp, __stack_top 

    // setup global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    // zero .bss
    la a0, __bss_start
    li a1, 0
    la a2, __bss_end
    sub a2, a2, a0
    call memset

    mv a0, t0
    mv a1, t1
    call kmain

    // jump to 0x0 = starting the firmware shell again
    li t0, 0
    jr t0