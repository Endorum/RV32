# test_trap.asm -- machine-mode traps: ecall/ebreak entry, mepc/mcause/mtval,
# the mstatus MIE/MPIE stack, mret, and real WFI in the poll idiom (wakes on
# a pending-and-enabled interrupt even though mstatus.MIE is off, no trap).
#
# A single generic handler stashes the trap CSRs into s-registers, steps mepc
# past the trapping instruction and returns; the main flow checks the stash.
#
# Result convention (inspect registers at halt, same for all tests):
#   x31 (t6) = 0x600DC0DE -> all checks passed, x28 = number of checks run
#   x31 (t6) = 0x0BADC0DE -> a check failed:
#       x28 (t3) = number of the failing check (1-based)
#       x29 (t4) = actual value
#       x30 (t5) = expected value
#
# x28-x31 belong to the harness; the handler stash lives in s2-s6.

#define PASS_CODE 0x600DC0DE
#define FAIL_CODE 0x0BADC0DE

#define CHECK(reg, expect) \
    addi x28, x28, 1;      \
    mv   x29, reg;         \
    li   x30, expect;      \
    bne  x29, x30, fail

// like CHECK, but the expected value is a register (for addresses via la)
#define CHECK_REG(reg, expreg) \
    addi x28, x28, 1;          \
    mv   x29, reg;             \
    mv   x30, expreg;          \
    bne  x29, x30, fail

    .option norvc
    .option arch, +zicsr
    .text
    .globl _start

_start:
    li   x28, 0

    # install the trap handler
    la   t0, trap_handler
    csrw mtvec, t0

    # --- ecall: enters the handler, mcause=11, mepc = address of the ecall,
    #     and execution continues after it
    li   s5, 0                  # handler-ran flag
ecall_site:
    ecall
    CHECK(s5, 1)                # 1  handler actually ran
    CHECK(s2, 11)               # 2  mcause = environment call from M-mode
    la   t1, ecall_site
    CHECK_REG(s3, t1)           # 3  mepc = the ecall itself, NOT behind it
    CHECK(s6, 0)                # 4  mtval = 0 for ecall

    # --- mstatus stack: MIE is pushed to MPIE on entry, popped by mret
    csrrsi x0, mstatus, 8       # MIE <- 1
    ecall
    andi t1, s4, 0x88           # in-handler MIE(3) / MPIE(7)
    CHECK(t1, 0x80)             # 5  handler runs with MIE off, MPIE holds the 1
    srli t1, s4, 11
    andi t1, t1, 3
    CHECK(t1, 3)                # 6  MPP = M (we trapped from M-mode)
    csrr t1, mstatus
    andi t1, t1, 8
    CHECK(t1, 8)                # 7  after mret MIE is restored
    csrrci x0, mstatus, 8       # MIE off again

    # --- ebreak: mcause=3, mepc and mtval both point at the ebreak
ebreak_site:
    ebreak
    CHECK(s2, 3)                # 8  mcause = breakpoint
    la   t1, ebreak_site
    CHECK_REG(s3, t1)           # 9  mepc  = ebreak address
    CHECK_REG(s6, t1)           # 10 mtval = ebreak address

    # --- wfi, the real one: with nothing pending it must SLEEP, so we arm
    #     the CLINT timer first. mstatus.MIE is off, MTIE in mie is on:
    #     the wake condition is (mip & mie) != 0 and ignores mstatus.MIE,
    #     so wfi wakes and falls through WITHOUT trapping (the poll idiom).
    li   s5, 0                  # handler-ran flag: must stay 0 this time
    li   a0, 0x4000C000         # CLINT mtime lives at 0x4000BFF8
    lw   a1, -8(a0)             # mtime (low word is enough here)
    addi a1, a1, 50             # deadline a few dozen ticks out
    li   a2, 0x40004000         # CLINT mtimecmp
    li   a3, -1
    sw   a3, 4(a2)              # cmp.hi = max FIRST (no torn 64-bit compare)
    sw   a1, 0(a2)              # cmp.lo = deadline
    sw   x0, 4(a2)              # cmp.hi = 0: armed
    li   a4, 0x80
    csrw mie, a4                # MTIE on (mstatus.MIE stays off)
    li   t1, 77
    wfi                         # sleeps until MTIP pends, then falls through
    CHECK(t1, 77)               # 11 alive after wfi, registers untouched
    CHECK(s5, 0)                # 12 no trap was taken (mstatus.MIE was off)
    li   a3, -1
    sw   a3, 4(a2)              # disarm: cmp.hi = max, MTIP drops
    csrw mie, x0                # MTIE off again

    j    pass

# generic handler: stash CSRs, flag, step past the trapping instruction.
# Clobbers t0 -- the main flow must not keep live state there across a trap.
trap_handler:
    csrr s2, mcause
    csrr s3, mepc
    csrr s4, mstatus
    csrr s6, mtval
    li   s5, 1
    csrr t0, mepc
    addi t0, t0, 4
    csrw mepc, t0
    mret

pass:
    li   x31, PASS_CODE
    j    halt

fail:
    li   x31, FAIL_CODE

halt:
    j    halt
