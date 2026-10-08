# test_loadstore.asm -- RV32I loads/stores: lb, lh, lw, lbu, lhu, sb, sh, sw
# Uses RAM at 0x20000000 (RAM_START in the RV32 emulator memory map).
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
#define MEM_BASE  0x20000000

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
    li   s0, MEM_BASE

    # sw / lw round trip
    li   t0, 0xDEADBEEF
    sw   t0, 0(s0)
    lw   t1, 0(s0)
    CHECK(t1, 0xDEADBEEF)             # 1

    # sb: bytes land little endian in memory
    li   t0, 0x78
    sb   t0, 4(s0)
    li   t0, 0x56
    sb   t0, 5(s0)
    li   t0, 0x34
    sb   t0, 6(s0)
    li   t0, 0x12
    sb   t0, 7(s0)
    lw   t1, 4(s0)
    CHECK(t1, 0x12345678)             # 2

    # lb / lbu sign extension, word 0x80FF7F01 at offset 8
    li   t0, 0x80FF7F01
    sw   t0, 8(s0)
    lb   t1, 8(s0)
    CHECK(t1, 1)                      # 3
    lb   t1, 9(s0)
    CHECK(t1, 0x7F)                   # 4
    lb   t1, 10(s0)
    CHECK(t1, -1)                     # 5   0xFF sign-extends to 0xFFFFFFFF
    lb   t1, 11(s0)
    CHECK(t1, 0xFFFFFF80)             # 6
    lbu  t1, 10(s0)
    CHECK(t1, 0xFF)                   # 7   lbu zero-extends
    lbu  t1, 11(s0)
    CHECK(t1, 0x80)                   # 8

    # lh / lhu sign extension, word 0x80007FFF at offset 12
    li   t0, 0x80007FFF
    sw   t0, 12(s0)
    lh   t1, 12(s0)
    CHECK(t1, 0x7FFF)                 # 9
    lh   t1, 14(s0)
    CHECK(t1, 0xFFFF8000)             # 10
    lhu  t1, 12(s0)
    CHECK(t1, 0x7FFF)                 # 11
    lhu  t1, 14(s0)
    CHECK(t1, 0x8000)                 # 12

    # sh: halves land little endian in memory
    li   t0, 0xABCD
    sh   t0, 16(s0)
    li   t0, 0x1234
    sh   t0, 18(s0)
    lw   t1, 16(s0)
    CHECK(t1, 0x1234ABCD)             # 13

    # sb must only touch one byte
    li   t0, 0x11111111
    sw   t0, 20(s0)
    li   t0, 0xFFFFFFAA
    sb   t0, 21(s0)
    lw   t1, 20(s0)
    CHECK(t1, 0x1111AA11)             # 14

    # sh must only touch the low half
    sw   x0, 24(s0)
    li   t0, 0xFFFF1234
    sh   t0, 24(s0)
    lw   t1, 24(s0)
    CHECK(t1, 0x00001234)             # 15

    # negative offsets
    addi s1, s0, 32
    li   t0, 0xCAFEBABE
    sw   t0, -4(s1)
    lw   t1, 28(s0)
    CHECK(t1, 0xCAFEBABE)             # 16
    lw   t1, -4(s1)
    CHECK(t1, 0xCAFEBABE)             # 17

    # load into the base register itself
    addi a0, s0, 36
    li   t0, 0x13371337
    sw   t0, 0(a0)
    lw   a0, 0(a0)
    CHECK(a0, 0x13371337)             # 18

    # the store keeps the value, later register changes must not matter
    li   t0, 55
    sw   t0, 40(s0)
    li   t0, 77
    lw   t1, 40(s0)
    CHECK(t1, 55)                     # 19

    # immediate offset range: +2044
    li   t0, 0x0DDBA11
    sw   t0, 2044(s0)
    lw   t1, 2044(s0)
    CHECK(t1, 0x0DDBA11)              # 20

    # immediate offset range: -2048 (s1 = base+2048, so s1-2048 == base+0)
    addi s1, s0, 2047
    addi s1, s1, 1
    li   t0, 0x5EED5EED
    sw   t0, -2048(s1)
    lw   t1, 0(s0)
    CHECK(t1, 0x5EED5EED)             # 21

    # reading constant data straight from ROM
    la   t0, ro_word
    lw   t1, 0(t0)
    CHECK(t1, 0x0C0FFEE0)             # 22

    j    pass

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt

    .section .rodata
ro_word:
    .word 0x0C0FFEE0
