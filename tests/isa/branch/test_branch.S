# test_branch.asm -- RV32I branches: beq, bne, blt, bge, bltu, bgeu
# Every branch is tested taken AND not taken; signed vs unsigned edge cases included.
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

    # beq taken
    li   t0, 5
    li   t1, 5
    li   s0, 1
    beq  t0, t1, 1f
    li   s0, 999
1:  CHECK(s0, 1)                      # 1

    # beq not taken: the fallthrough instruction must execute
    li   t0, 5
    li   t1, 6
    li   s0, 1
    beq  t0, t1, 1f
    li   s0, 2
1:  CHECK(s0, 2)                      # 2

    # bne taken
    li   t0, 5
    li   t1, 6
    li   s0, 1
    bne  t0, t1, 1f
    li   s0, 999
1:  CHECK(s0, 1)                      # 3

    # bne not taken
    li   t0, 7
    li   t1, 7
    li   s0, 1
    bne  t0, t1, 1f
    li   s0, 2
1:  CHECK(s0, 2)                      # 4

    # blt taken: -1 < 1 signed
    li   t0, -1
    li   t1, 1
    li   s0, 1
    blt  t0, t1, 1f
    li   s0, 999
1:  CHECK(s0, 1)                      # 5

    # blt not taken: 1 < -1 is false signed
    li   t0, 1
    li   t1, -1
    li   s0, 1
    blt  t0, t1, 1f
    li   s0, 2
1:  CHECK(s0, 2)                      # 6

    # blt not taken on equal values
    li   t0, 3
    li   t1, 3
    li   s0, 1
    blt  t0, t1, 1f
    li   s0, 2
1:  CHECK(s0, 2)                      # 7

    # bge taken: 1 >= -1 signed
    li   t0, 1
    li   t1, -1
    li   s0, 1
    bge  t0, t1, 1f
    li   s0, 999
1:  CHECK(s0, 1)                      # 8

    # bge taken on equal values
    li   t0, 3
    li   t1, 3
    li   s0, 1
    bge  t0, t1, 1f
    li   s0, 999
1:  CHECK(s0, 1)                      # 9

    # bge not taken: -1 >= 1 is false signed
    li   t0, -1
    li   t1, 1
    li   s0, 1
    bge  t0, t1, 1f
    li   s0, 2
1:  CHECK(s0, 2)                      # 10

    # bltu taken: unsigned, so -1 is the biggest value: 1 < 0xFFFFFFFF
    li   t0, 1
    li   t1, -1
    li   s0, 1
    bltu t0, t1, 1f
    li   s0, 999
1:  CHECK(s0, 1)                      # 11

    # bltu not taken: 0xFFFFFFFF < 1 is false unsigned
    li   t0, -1
    li   t1, 1
    li   s0, 1
    bltu t0, t1, 1f
    li   s0, 2
1:  CHECK(s0, 2)                      # 12

    # bgeu taken: 0xFFFFFFFF >= 1 unsigned
    li   t0, -1
    li   t1, 1
    li   s0, 1
    bgeu t0, t1, 1f
    li   s0, 999
1:  CHECK(s0, 1)                      # 13

    # bgeu not taken: 1 >= 0xFFFFFFFF is false unsigned
    li   t0, 1
    li   t1, -1
    li   s0, 1
    bgeu t0, t1, 1f
    li   s0, 2
1:  CHECK(s0, 2)                      # 14

    # comparing a register with itself is always equal
    li   t0, 42
    li   s0, 1
    beq  t0, t0, 1f
    li   s0, 999
1:  CHECK(s0, 1)                      # 15

    # backward branch: loop computes 5+4+3+2+1
    li   t0, 5
    li   t1, 0
1:  add  t1, t1, t0
    addi t0, t0, -1
    bne  t0, x0, 1b
    CHECK(t1, 15)                     # 16

    j    pass

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt
