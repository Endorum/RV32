/*
 * Linker script for the ISA test ROMs -- each test is a bare ROM image that
 * starts at the reset vector. Addresses come from platform/memory_map.h.
 */

#include "memory_map.h"

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
  ROM (rx):  ORIGIN = ROM_START, LENGTH = ROM_SIZE
  RAM (rwx): ORIGIN = RAM_START, LENGTH = RAM_SIZE
}

SECTIONS {
  .text   : { *(.text.init) *(.text*) } > ROM
  .rodata : { *(.rodata*) *(.srodata*) } > ROM
  .data   : { *(.data*) *(.sdata*) } > RAM AT> ROM
  .bss    : { *(.bss*) *(.sbss*) *(COMMON) } > RAM
}
