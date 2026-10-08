# test_alu_reg.asm -- RV32I register ALU ops: add, sub, slt, sltu, xor, or, and + x0 behavior
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

    # add
    li   t0, 3
    li   t1, 4
    add  t2, t0, t1
    CHECK(t2, 7)                      # 1

    li   t0, -5
    li   t1, 3
    add  t2, t0, t1
    CHECK(t2, -2)                     # 2

    li   t0, 0x80000000
    li   t1, 0x80000000
    add  t2, t0, t1
    CHECK(t2, 0)                      # 3   wraps around

    li   t0, 0x7FFFFFFF
    li   t1, 1
    add  t2, t0, t1
    CHECK(t2, 0x80000000)             # 4

    li   t0, 7
    add  t0, t0, t0
    CHECK(t0, 14)                     # 5   rd == rs1 == rs2

    # sub
    li   t0, 5
    li   t1, 7
    sub  t2, t0, t1
    CHECK(t2, -2)                     # 6

    li   t0, 0x80000000
    sub  t2, x0, t0
    CHECK(t2, 0x80000000)             # 7   negating INT_MIN wraps to itself

    li   t0, 9
    sub  t0, t0, t0
    CHECK(t0, 0)                      # 8

    # slt (signed)
    li   t0, -1
    li   t1, 1
    slt  t2, t0, t1
    CHECK(t2, 1)                      # 9

    slt  t2, t1, t0
    CHECK(t2, 0)                      # 10

    slt  t2, t0, t0
    CHECK(t2, 0)                      # 11  equal is not less

    li   t0, 0x80000000
    li   t1, 0x7FFFFFFF
    slt  t2, t0, t1
    CHECK(t2, 1)                      # 12  INT_MIN < INT_MAX

    # sltu (unsigned)
    li   t0, 1
    li   t1, -1
    sltu t2, t0, t1
    CHECK(t2, 1)                      # 13  1 < 0xFFFFFFFF

    sltu t2, t1, t0
    CHECK(t2, 0)                      # 14

    li   t0, 5
    sltu t2, x0, t0
    CHECK(t2, 1)                      # 15  the snez pattern

    sltu t2, x0, x0
    CHECK(t2, 0)                      # 16

    # xor / or / and
    li   t0, 0xAAAAAAAA
    li   t1, 0x55555555
    xor  t2, t0, t1
    CHECK(t2, 0xFFFFFFFF)             # 17

    xor  t2, t0, t0
    CHECK(t2, 0)                      # 18

    li   t0, 0xFF00FF00
    li   t1, 0x0FF00FF0
    or   t2, t0, t1
    CHECK(t2, 0xFFF0FFF0)             # 19

    and  t2, t0, t1
    CHECK(t2, 0x0F000F00)             # 20

    # x0 is hardwired to zero: writes are discarded
    li   t0, 123
    add  x0, t0, t0
    CHECK(x0, 0)                      # 21

    addi x0, x0, 55
    CHECK(x0, 0)                      # 22

    add  t1, x0, x0
    CHECK(t1, 0)                      # 23

    j    pass

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt
