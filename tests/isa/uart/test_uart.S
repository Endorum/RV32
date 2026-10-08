# test_uart.asm -- 16550-subset UART at 0x50000000: LSR idle bits, the
# xv6-style init dance through DLAB (divisor latch bank switch), divisor
# readback, IER isolation from DLM, RBR-on-empty, and finally visible TX
# output through the polled putchar pattern every later firmware will use.
#
# RX cannot be self-tested from the guest (no loopback); that path is pinned
# by the C++ device tests in RV32/tests/test_uart.cpp.
#
# Result convention (inspect registers at halt, same for all tests):
#   x31 (t6) = 0x600DC0DE -> all checks passed, x28 = number of checks run
#   x31 (t6) = 0x0BADC0DE -> a check failed:
#       x28 (t3) = number of the failing check (1-based)
#       x29 (t4) = actual value
#       x30 (t5) = expected value
#
# x28-x31 belong to the harness; t0 holds the UART base throughout.

#define PASS_CODE 0x600DC0DE
#define FAIL_CODE 0x0BADC0DE

#define UART_BASE 0x50000000

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
    li   t0, UART_BASE

    # --- reset state: transmitter idle (THRE|TEMT), no receive data
    lbu  t1, 5(t0)              # LSR
    CHECK(t1, 0x60)             # 1  idle, DR clear

    # --- RBR on an empty FIFO reads as 0 (defined, no data invented)
    lbu  t1, 0(t0)
    CHECK(t1, 0x00)             # 2

    # --- give IER a distinctive value BEFORE touching DLAB, so we can
    #     prove the divisor writes land in DLL/DLM and not in IER
    li   t1, 0x0D
    sb   t1, 1(t0)              # IER = 0x0D

    # --- the canonical init: divisor via the DLAB bank switch
    li   t1, 0x80
    sb   t1, 3(t0)              # LCR = DLAB on: offsets 0/1 are now DLL/DLM
    li   t1, 0x03
    sb   t1, 0(t0)              # DLL = 3 (38400 baud; value is decoration)
    li   t1, 0x5A
    sb   t1, 1(t0)              # DLM = 0x5A (distinctive on purpose)

    lbu  t1, 0(t0)
    CHECK(t1, 0x03)             # 3  DLL reads back through the latch
    lbu  t1, 1(t0)
    CHECK(t1, 0x5A)             # 4  DLM reads back, this is NOT IER

    li   t1, 0x03
    sb   t1, 3(t0)              # LCR = 8N1, DLAB off: banks switch back
    lbu  t1, 3(t0)
    CHECK(t1, 0x03)             # 5  LCR readback

    lbu  t1, 1(t0)
    CHECK(t1, 0x0D)             # 6  IER untouched by the DLM write

    li   t1, 0x07
    sb   t1, 2(t0)              # FCR: FIFOs on + clear both
    lbu  t1, 5(t0)
    CHECK(t1, 0x60)             # 7  still idle, still no data

    sb   x0, 1(t0)              # IER = 0 again (no interrupts, we poll)

    # --- and the payoff: print over TX with the real polled putchar loop
    la   a0, msg
puts_loop:
    lbu  a1, 0(a0)
    beqz a1, puts_done
1:  lbu  t1, 5(t0)              # poll LSR until transmitter ready
    andi t1, t1, 0x20
    beqz t1, 1b
    sb   a1, 0(t0)              # THR
    addi a0, a0, 1
    j    puts_loop
puts_done:

    j    pass

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt

    # string lives in .text after the halt loop; execution never reaches it
msg:
    .asciz "uart ok\n"
