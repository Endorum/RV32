

// interrupt bit not set
enum class EXCEPTION_CODE : u32{
  INSTR_ADDR_MISALIGNED = 0,
  INSTR_ACCESS_FAULT    = 1,
  ILLEGAL_INSTRUCTION   = 2,
  BREAKPOINT            = 3,
  LOAD_ADDR_MISALIGNED  = 4,
  LOAD_ACCESS_FAULT     = 5,
  STORE_ADDR_MISALIGNED = 6,
  STORE_ACCESS_FAULT    = 7,
  ECALL_FROM_U_MODE     = 8,
  ECALL_FROM_S_MODE     = 9,
  ECALL_FROM_M_MODE     = 11,
  INSTR_PAGE_FAULT      = 12,
  LOAD_PAGE_FAULT       = 13,
  STORE_PAGE_FAULT      = 15,
  DOUBLE_TRAP           = 16,
  SOFTWARE_CHECK        = 18,
  HARDWARE_ERROR        = 19,
};

// interrupt bit set
enum class INTERRUPT_CODE : u32 {
  SUP_SOFTWARE_INT    = 0x80000001,
  MACH_SOFTWARE_INT   = 0x80000003,
  SUP_TIMER_INT       = 0x80000005,
  MACH_TIMER_INT      = 0x80000007,
  SUP_EXT_INT         = 0x80000009,
  MACH_EXT_INT        = 0x8000000B,
  COUNT_OVERFLOW_INT  = 0x8000000D,
};


aus :

| Interrupt | Exception Code | Description                    |
| --------: | -------------: | ------------------------------ |
|         1 |              0 | Reserved                       |
|         1 |              1 | Supervisor software interrupt  |
|         1 |              2 | Reserved                       |
|         1 |              3 | Machine software interrupt     |
|         1 |              4 | Reserved                       |
|         1 |              5 | Supervisor timer interrupt     |
|         1 |              6 | Reserved                       |
|         1 |              7 | Machine timer interrupt        |
|         1 |              8 | Reserved                       |
|         1 |              9 | Supervisor external interrupt  |
|         1 |             10 | Reserved                       |
|         1 |             11 | Machine external interrupt     |
|         1 |             12 | Reserved                       |
|         1 |             13 | Counter-overflow interrupt     |
|         1 |          14–15 | Reserved                       |
|         1 |            ≥16 | Designated for platform use    |
|         0 |              0 | Instruction address misaligned |
|         0 |              1 | Instruction access fault       |
|         0 |              2 | Illegal instruction            |
|         0 |              3 | Breakpoint                     |
|         0 |              4 | Load address misaligned        |
|         0 |              5 | Load access fault              |
|         0 |              6 | Store/AMO address misaligned   |
|         0 |              7 | Store/AMO access fault         |
|         0 |              8 | Environment call from U-mode   |
|         0 |              9 | Environment call from S-mode   |
|         0 |             10 | Reserved                       |
|         0 |             11 | Environment call from M-mode   |
|         0 |             12 | Instruction page fault         |
|         0 |             13 | Load page fault                |
|         0 |             14 | Reserved                       |
|         0 |             15 | Store/AMO page fault           |
|         0 |             16 | Double trap                    |
|         0 |             17 | Reserved                       |
|         0 |             18 | Software check                 |
|         0 |             19 | Hardware error                 |
|         0 |          20–23 | Reserved                       |
|         0 |          24–31 | Designated for custom use      |
|         0 |          32–47 | Reserved                       |
|         0 |          48–63 | Designated for custom use      |
|         0 |            ≥64 | Reserved                       |
