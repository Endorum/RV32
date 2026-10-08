# test_jump.asm -- RV32I jumps: jal, jalr (link value, target computation, bit-0 clearing)
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

    # jal forward: the poison instruction must be skipped
    li   s0, 1
    jal  x0, 1f
    addi s0, x0, 999
1:  CHECK(s0, 1)                      # 1

    # jal writes pc+4 into rd (measured as a delta from auipc)
    auipc t0, 0
    jal   ra, 1f
    addi  s0, x0, 999
1:  sub   t1, ra, t0
    CHECK(t1, 8)                      # 2   ra = address of the jal + 4

    # jal backward
    li   s0, 0
    j    2f
1:  addi s0, s0, 5
    j    3f
2:  addi s0, s0, 10
    jal  x0, 1b
    addi s0, x0, 999
3:  CHECK(s0, 15)                     # 3

    # call/ret round trip (ret is jalr x0, 0(ra))
    li   a0, 0
    jal  ra, func
    CHECK(a0, 42)                     # 4

    # jalr writes pc+4 into rd and jumps to rs1+offset
    la    t0, 1f
    li    s0, 1
    auipc t1, 0
    jalr  ra, 0(t0)
    addi  s0, x0, 999
1:  sub   t2, ra, t1
    CHECK(t2, 8)                      # 5   ra = address of the jalr + 4
    CHECK(s0, 1)                      # 6   poison was skipped

    # jalr must clear bit 0 of the computed target
    la   t0, 1f
    ori  t0, t0, 1
    li   s0, 1
    jalr x0, 0(t0)
    addi s0, x0, 999
1:  CHECK(s0, 1)                      # 7

    # jalr with a positive offset: target is jt_base+8, both poisons skipped
    la   t0, jt_base
    li   s0, 1
    jalr x0, 8(t0)
jt_base:
    addi s0, x0, 999
    addi s0, x0, 998
    CHECK(s0, 1)                      # 8

    # jalr with a negative offset: target is jn_end-8 == jn_target
    la   t0, jn_end
    li   s0, 1
    jalr x0, -8(t0)
    addi s0, x0, 999
jn_target:
    addi s0, s0, 1
    j    1f
jn_end:
    addi s0, x0, 997
1:  CHECK(s0, 2)                      # 9

    j    pass

func:
    li   a0, 42
    ret

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt
