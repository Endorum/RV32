# test_shift.asm -- RV32I shifts: slli, srli, srai, sll, srl, sra
# Register shifts only use the low 5 bits of rs2 -- that part catches a lot of bugs.
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
    .text
    .globl _start

_start:
    li   x28, 0

    # slli
    li   t0, 1
    slli t1, t0, 0
    CHECK(t1, 1)                      # 1

    slli t1, t0, 1
    CHECK(t1, 2)                      # 2

    slli t1, t0, 31
    CHECK(t1, 0x80000000)             # 3

    li   t0, -1
    slli t1, t0, 4
    CHECK(t1, 0xFFFFFFF0)             # 4

    li   t0, 0x81234567
    slli t1, t0, 4
    CHECK(t1, 0x12345670)             # 5   top bits fall off

    # srli (logical, zero fill)
    li   t0, 0x80000000
    srli t1, t0, 31
    CHECK(t1, 1)                      # 6

    li   t0, -1
    srli t1, t0, 4
    CHECK(t1, 0x0FFFFFFF)             # 7

    li   t0, 0x80000000
    srli t1, t0, 0
    CHECK(t1, 0x80000000)             # 8

    # srai (arithmetic, sign fill)
    li   t0, 0x80000000
    srai t1, t0, 31
    CHECK(t1, -1)                     # 9

    li   t0, 0xF0000000
    srai t1, t0, 4
    CHECK(t1, 0xFF000000)             # 10

    li   t0, 0x7FFFFFFF
    srai t1, t0, 4
    CHECK(t1, 0x07FFFFFF)             # 11  positive values shift in zeros

    li   t0, -16
    srai t1, t0, 2
    CHECK(t1, -4)                     # 12

    # sll: only the low 5 bits of rs2 count
    li   t0, 1
    li   t1, 32
    sll  t2, t0, t1
    CHECK(t2, 1)                      # 13  shift by 32 acts as shift by 0

    li   t1, 33
    sll  t2, t0, t1
    CHECK(t2, 2)                      # 14  shift by 33 acts as shift by 1

    li   t1, 63
    sll  t2, t0, t1
    CHECK(t2, 0x80000000)             # 15  shift by 63 acts as shift by 31

    li   t0, 0xFF
    li   t1, 8
    sll  t2, t0, t1
    CHECK(t2, 0xFF00)                 # 16

    # srl
    li   t0, 0x80000000
    li   t1, 32
    srl  t2, t0, t1
    CHECK(t2, 0x80000000)             # 17  shift by 32 acts as shift by 0

    li   t1, 33
    srl  t2, t0, t1
    CHECK(t2, 0x40000000)             # 18

    li   t0, 0xF0F0F0F0
    li   t1, 4
    srl  t2, t0, t1
    CHECK(t2, 0x0F0F0F0F)             # 19

    # sra
    li   t0, 0x80000000
    li   t1, 31
    sra  t2, t0, t1
    CHECK(t2, -1)                     # 20

    li   t0, 0xFFFF0000
    li   t1, 8
    sra  t2, t0, t1
    CHECK(t2, 0xFFFFFF00)             # 21

    li   t0, 0x7F000000
    li   t1, 24
    sra  t2, t0, t1
    CHECK(t2, 0x7F)                   # 22

    j    pass

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt
