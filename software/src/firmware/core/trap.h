#pragma once

#include "stdint.h"
#include "tf.h"

enum EXCEPTION_CODE {
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

enum INTERRUPT_CODE {
  SUP_SOFTWARE_INT    = 0x80000001,
  MACH_SOFTWARE_INT   = 0x80000003,
  SUP_TIMER_INT       = 0x80000005,
  MACH_TIMER_INT      = 0x80000007,
  SUP_EXT_INT         = 0x80000009,
  MACH_EXT_INT        = 0x8000000B,
  COUNT_OVERFLOW_INT  = 0x8000000D,
};

typedef struct trap_frame {
    uint32_t x[32];     // x[n] = register xn, x[0] stays unused
    uint32_t mepc;      // Offset 32*4 = 128
    uint32_t mstatus;   // Offset 33*4 = 132
}trap_frame_t; // 136 Bytes -> round to 144 Bytes



void trap(trap_frame_t* tf);
void exc_handler(trap_frame_t* tf, uint32_t cause, uint32_t tval);
void handle_ecall(trap_frame_t* tf, uint32_t cause);
void msi_handler(trap_frame_t* tf);
void mti_handler(trap_frame_t* tf);


