# test_zicsr.asm -- Zicsr instructions: csrrw, csrrs, csrrc, csrrwi, csrrsi, csrrci
# Uses mscratch (0x340) as scratch CSR plus mepc/mtvec/mcause for independence checks.
# Zicsr is enabled per-file via .option arch so build.sh can stay on plain rv32i.
#
# Result convention (inspect registers at halt, same for all tests):
#   x31 (t6) = 0x600DC0DE -> all checks passed, x28 = number of checks run
#   x31 (t6) = 0x0BADC0DE -> a check failed:
#       x28 (t3) = number of the failing check (1-based)
#       x29 (t4) = actual value
#       x30 (t5) = expected value
#
# x28-x31 belong to the harness, everything else is free.

#define PASS_CODE 0x600DC0DE
#define FAIL_CODE 0x0BADC0DE

#define CHECK(reg, expect) \
    addi x28, x28, 1;      \
    mv   x29, reg;         \
    li   x30, expect;      \
    bne  x29, x30, fail

    .option norvc
    .option arch, +zicsr
    .text
    .globl _start

_start:
    li   x28, 0

    # csrrw writes, csrr (csrrs rd, csr, x0) reads back
    li    t0, 0x12345678
    csrrw x0, mscratch, t0
    csrr  t1, mscratch
    CHECK(t1, 0x12345678)             # 1

    # csrrw returns the OLD value while writing the new one
    li    t0, 0xAAAA5555
    csrrw t1, mscratch, t0
    CHECK(t1, 0x12345678)             # 2
    csrr  t2, mscratch
    CHECK(t2, 0xAAAA5555)             # 3

    # csrrw with rd == rs1 is a clean swap
    li    t0, 0x0BADF00D
    csrrw t0, mscratch, t0
    CHECK(t0, 0xAAAA5555)             # 4
    csrr  t1, mscratch
    CHECK(t1, 0x0BADF00D)             # 5

    # csrrs returns the old value and ORs rs1 into the CSR
    li    t0, 0xF0F0F0F0
    csrrw x0, mscratch, t0
    li    t0, 0x0F0F0F0F
    csrrs t1, mscratch, t0
    CHECK(t1, 0xF0F0F0F0)             # 6
    csrr  t2, mscratch
    CHECK(t2, 0xFFFFFFFF)             # 7

    # csrrs with rs1 == x0 only reads, the CSR must not change
    csrrs t1, mscratch, x0
    CHECK(t1, 0xFFFFFFFF)             # 8
    csrr  t2, mscratch
    CHECK(t2, 0xFFFFFFFF)             # 9

    # csrrc returns the old value and clears the rs1 bits
    li    t0, 0xFF
    csrrc t1, mscratch, t0
    CHECK(t1, 0xFFFFFFFF)             # 10
    csrr  t2, mscratch
    CHECK(t2, 0xFFFFFF00)             # 11

    # csrrc with rs1 == x0 only reads, the CSR must not change
    csrrc t1, mscratch, x0
    CHECK(t1, 0xFFFFFF00)             # 12
    csrr  t2, mscratch
    CHECK(t2, 0xFFFFFF00)             # 13

    # csrrwi: the 5-bit immediate is ZERO-extended (31 stays 31)
    csrrwi t1, mscratch, 31
    CHECK(t1, 0xFFFFFF00)             # 14  old value comes back
    csrr   t2, mscratch
    CHECK(t2, 31)                     # 15

    csrrwi t1, mscratch, 0
    CHECK(t1, 31)                     # 16
    csrr   t2, mscratch
    CHECK(t2, 0)                      # 17

    # csrrsi sets immediate bits
    li     t0, 0x100
    csrrw  x0, mscratch, t0
    csrrsi t1, mscratch, 0x15
    CHECK(t1, 0x100)                  # 18
    csrr   t2, mscratch
    CHECK(t2, 0x115)                  # 19

    # csrrci clears immediate bits
    csrrci t1, mscratch, 0x05
    CHECK(t1, 0x115)                  # 20
    csrr   t2, mscratch
    CHECK(t2, 0x110)                  # 21

    # csrrsi/csrrci with uimm == 0 must leave the CSR alone
    csrrsi t1, mscratch, 0
    CHECK(t1, 0x110)                  # 22
    csrrci t1, mscratch, 0
    CHECK(t1, 0x110)                  # 23
    csrr   t2, mscratch
    CHECK(t2, 0x110)                  # 24

    # different CSRs are independent registers
    li    t0, 0x11111111
    csrrw x0, mepc, t0
    li    t0, 0x22222222
    csrrw x0, mtvec, t0
    li    t0, 0x33333333
    csrrw x0, mcause, t0
    csrr  t1, mepc
    CHECK(t1, 0x11111111)             # 25
    csrr  t1, mtvec
    CHECK(t1, 0x22222222)             # 26
    csrr  t1, mcause
    CHECK(t1, 0x33333333)             # 27
    csrr  t1, mscratch
    CHECK(t1, 0x110)                  # 28  mscratch untouched by the writes above

    j    pass

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt
