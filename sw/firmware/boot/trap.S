.global trap_entry

#include "tf.h"

.align 2
trap_entry:
    # change to trap stack
    csrrw sp, mscratch, sp  # sp = Trap-Stack, mscratch = old sp
    addi sp, sp, -TF_SIZE

    # save registers
    .irp n, 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        sw x\n, \n*4(sp)
    .endr

    csrr t0, mscratch   # t0 is already saved
    sw t0, 2*4(sp)      # old sp now as x2 in Frame

    # save CSRs
    csrr t0, mepc
    sw t0, TF_MEPC(sp)
    csrr t0, mstatus
    sw t0, TF_MSTATUS(sp)

    # call C function trap(struct trap_frame *tf)
    mv a0, sp # 1. arg = pointer to frame
    call trap

    # get CSRs back from the frame
    lw t0, TF_MEPC(sp)
    csrw mepc, t0
    lw t0, TF_MSTATUS(sp)
    csrw mstatus, t0

    # set mscratch to trap stack
    addi t0, sp, TF_SIZE        # top of trap stack
    csrw mscratch, t0

    # get registers back, sp at the end

    .irp n, 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        lw x\n, \n*4(sp)
    .endr

    # last load instr. after this sp does not point to the frame anymore
    lw sp, 8(sp) 

    # return
    mret
