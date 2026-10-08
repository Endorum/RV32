# test_alu_imm.asm -- RV32I immediate ALU ops: addi, slti, sltiu, xori, ori, andi, lui, auipc
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

    # addi
    addi t0, x0, 5
    CHECK(t0, 5)                      # 1

    addi t0, t0, -3
    CHECK(t0, 2)                      # 2

    li   t0, 0
    addi t0, t0, -1
    CHECK(t0, -1)                     # 3   result is 0xFFFFFFFF

    li   t0, 0x7FFFFFFF
    addi t0, t0, 1
    CHECK(t0, 0x80000000)             # 4   overflow wraps, no trap

    addi t0, x0, 2047
    addi t0, t0, 2047
    CHECK(t0, 4094)                   # 5   max positive immediate, twice

    addi t0, x0, -2048
    CHECK(t0, -2048)                  # 6   most negative immediate

    # slti (signed compare)
    li   t0, -5
    slti t1, t0, -4
    CHECK(t1, 1)                      # 7   -5 < -4

    li   t0, -4
    slti t1, t0, -4
    CHECK(t1, 0)                      # 8   equal is not less

    li   t0, -3000
    slti t1, t0, -2048
    CHECK(t1, 1)                      # 9

    li   t0, 5
    slti t1, t0, 2047
    CHECK(t1, 1)                      # 10

    # sltiu (unsigned compare, immediate is still sign-extended first)
    li    t0, 0
    sltiu t1, t0, 1
    CHECK(t1, 1)                      # 11  the seqz pattern

    li    t0, 3
    sltiu t1, t0, 1
    CHECK(t1, 0)                      # 12

    li    t0, 5
    sltiu t1, t0, -1
    CHECK(t1, 1)                      # 13  imm -1 acts as 0xFFFFFFFF unsigned

    li    t0, -1
    sltiu t1, t0, -1
    CHECK(t1, 0)                      # 14  0xFFFFFFFF is not < 0xFFFFFFFF

    # xori
    li   t0, 0xF0F0F0F0
    xori t1, t0, 0xFF
    CHECK(t1, 0xF0F0F00F)             # 15

    li   t0, 0x12345678
    xori t1, t0, -1
    CHECK(t1, 0xEDCBA987)             # 16  xori with -1 is bitwise not

    # ori
    li   t0, 0x12340000
    ori  t1, t0, 0x5FF
    CHECK(t1, 0x123405FF)             # 17

    li   t0, 0x12345678
    ori  t1, t0, -256
    CHECK(t1, 0xFFFFFF78)             # 18  imm sign-extends to 0xFFFFFF00

    # andi
    li   t0, -1
    andi t1, t0, 0x7FF
    CHECK(t1, 0x7FF)                  # 19

    li   t0, 0x12345678
    andi t1, t0, -16
    CHECK(t1, 0x12345670)             # 20  imm sign-extends to 0xFFFFFFF0

    # lui
    lui  t0, 0xFFFFF
    CHECK(t0, 0xFFFFF000)             # 21

    lui  t0, 0x80000
    CHECK(t0, 0x80000000)             # 22

    lui  t0, 1
    CHECK(t0, 0x1000)                 # 23

    # lui + addi with a negative low part must still compose correctly
    lui  t0, 0xDEADC
    addi t0, t0, -273
    CHECK(t0, 0xDEADBEEF)             # 24

    # auipc: only deltas are checked so the test does not depend on its address
    auipc t0, 0
    auipc t1, 0
    sub   t2, t1, t0
    CHECK(t2, 4)                      # 25  consecutive auipc results differ by 4

    auipc t0, 0
    auipc t1, 1
    sub   t2, t1, t0
    CHECK(t2, 0x1004)                 # 26  (1 << 12) plus the 4-byte pc step

    j    pass

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt
