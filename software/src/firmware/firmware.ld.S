/*
 * Firmware linker script -- preprocessed by cpp before linking, so the
 * addresses come from the single source of truth:
 *   platform/memory_map.h   ROM_*            (hardware)
 *   abi/ram_layout.h        RAM_FIRM_*       (software convention)
 *
 * The firmware runs in place from ROM. Only its writable data lives in RAM,
 * inside the firmware's own RAM region -- everything above belongs to the
 * kernel (see ram_layout.h).
 *
 *   ROM  ROM_START       .text (boot code first), .rodata, initial .data
 *   RAM  RAM_FIRM_START  .data (copied from ROM by boot.S), .bss,
 *                        stack growing down from the end of RAM_FIRM
 */

#include "memory_map.h"
#include "ram_layout.h"

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
  ROM (rx):  ORIGIN = ROM_START,      LENGTH = ROM_SIZE
  RAM (rwx): ORIGIN = RAM_FIRM_START, LENGTH = RAM_FIRM_SIZE
}

SECTIONS {
  .text : {
    *(.text.init)   /* boot code first -> first instruction at ROM_START */
    *(.text*)
  } > ROM

  .rodata : { *(.rodata*) *(.srodata*) } > ROM

  .data : ALIGN(4) {
    __data_start = .;

    *(.data*)

    PROVIDE(__global_pointer$ = . + 0x800);

    *(.sdata*)

    . = ALIGN(4);
    __data_end = .;
  } > RAM AT> ROM   /* VMA in RAM, LMA in ROM */
  __data_lma = LOADADDR(.data);   /* source address for the copy in boot.S */

  .bss : ALIGN(4) {
    __bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > RAM           /* no bytes in the image, only space */

  _end = .;
  __stack_top = ORIGIN(RAM) + LENGTH(RAM);   /* top of RAM_FIRM */
}
