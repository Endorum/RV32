    .section .text
    .globl _start

_start:
############################################################
# 1. LUI, AUIPC
############################################################
    lui     x1, 0x12345        # Test LUI
    auipc   x2, 0x10           # Test AUIPC

############################################################
# 2. JAL, JALR (control flow)
############################################################
    la      x3, label_after_jal
    jal     x4, label_jal      # x4 = return addr
label_jal:
    addi    x5, x0, 5          # executed after jump
    jalr    x6, 0(x3)          # jump to label_after_jal

label_after_jal:
    addi    x7, x0, 7

############################################################
# 3. Branch Instructions: BEQ/BNE/BLT/BGE/BLTU/BGEU
############################################################
    addi    x8, x0, 10
    addi    x9, x0, 10
    addi    x10, x0, 5

    beq     x8, x9, branch_eq_pass
    addi    x11, x0, 99        # fail
branch_eq_pass:
    bne     x8, x10, branch_ne_pass
    addi    x11, x0, 98        # fail
branch_ne_pass:
    blt     x10, x8, branch_lt_pass
    addi    x11, x0, 97
branch_lt_pass:
    bge     x8, x10, branch_ge_pass
    addi    x11, x0, 96
branch_ge_pass:
    bltu    x10, x8, branch_ltu_pass
    addi    x11, x0, 95
branch_ltu_pass:
    bgeu    x8, x10, branch_geu_pass
    addi    x11, x0, 94
branch_geu_pass:

############################################################
# 4. Loads: LB/LH/LW/LBU/LHU
############################################################
    la      x12, data_area
    lb      x13, 0(x12)
    lh      x14, 2(x12)
    lw      x15, 4(x12)
    lbu     x16, 1(x12)
    lhu     x17, 2(x12)

############################################################
# 5. Stores: SB/SH/SW
############################################################
    la      x18, data_area
    addi    x19, x0, 0x11
    sb      x19, 8(x18)
    
    lui     x19, 0x2        # upper 20 bits = 0x2
    addi    x19, x19, 0x233 # lower 12 bits = 0x233
    sh      x19, 10(x18)
    
    lui     x19, 0x11223    # upper 20 bits
    addi    x19, x19, 0x344 # lower 12 bits MUST be sign-extended range
    sw      x19, 12(x18)

############################################################
# 6. ALU Immediate: ADDI/SLTI/SLTIU/XORI/ORI/ANDI/SLLI/SRLI/SRAI
############################################################
    addi    x20, x0, 5
    slti    x21, x20, 10
    sltiu   x22, x20, 10
    xori    x23, x20, 0xF
    ori     x24, x20, 0xF
    andi    x25, x20, 0xF

    slli    x26, x20, 2
    srli    x27, x20, 1
    srai    x28, x20, 1

############################################################
# 7. ALU Register: ADD/SUB/SLL/SLT/SLTU/XOR/SRL/SRA/OR/AND
############################################################
    addi    x29, x0, 8
    addi    x30, x0, 3

    add     x10, x29, x30
    sub     x11, x29, x30
    sll     x12, x29, x30
    slt     x13, x29, x30
    sltu    x14, x30, x29
    xor     x15, x29, x30
    srl     x16, x29, x30
    sra     x17, x29, x30
    or      x18, x29, x30
    and     x19, x29, x30

############################################################
# 8. Misc: FENCE, ECALL, EBREAK
############################################################
    fence
    ecall
    ebreak

# End marker (loop forever)
end_loop:
    jal     x0, end_loop

############################################################
# Data region for load/store tests
############################################################
    .section .data
data_area:
    .byte 0xAA, 0xBB          # bytes
    .half 0xCCDD              # half
    .word 0x11223344          # word
    .space 16                 # store destination area
