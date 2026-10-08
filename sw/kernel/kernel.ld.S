/*
 * Kernel linker script -- preprocessed by cpp before linking, so the
 * addresses come from the single source of truth:
 *   abi/ram_layout.h   RAM_KERN_*   (where the firmware loads the kernel)
 *
 * Boot contract (see sw/common/abi/info.md): the firmware copies the image
 * from the HD to RAM_KERN_START and jumps to its first byte. So the image
 * is linked for RAM_KERN_START and _start (.text.init) must come first.
 *
 * The image is a flat binary: it contains .text, .rodata and .data, but
 * NOT .bss -- the kernel has to zero __bss_start..__bss_end itself.
 *
 *   KRAM  RAM_KERN_START  .text.init (_start), .text, .rodata, .data, .bss,
 *                         stack growing down from the end of RAM_KERN
 */

#include "ram_layout.h"

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
  KRAM (rwx): ORIGIN = RAM_KERN_START, LENGTH = RAM_KERN_SIZE
}

SECTIONS {
  .text : {
    *(.text.init)   /* entry first -> first instruction at RAM_KERN_START */
    *(.text*)
  } > KRAM

  .rodata : { *(.rodata*) *(.srodata*) } > KRAM

  .data : ALIGN(4) {
    __data_start = .;

    *(.data*)

    PROVIDE(__global_pointer$ = . + 0x800);

    *(.sdata*)

    . = ALIGN(4);
    __data_end = .;
  } > KRAM          /* already in place, no copy needed */

  .bss : ALIGN(4) {
    __bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > KRAM          /* not in the image -> zero it at startup */

  _end = .;
  __stack_top = ORIGIN(KRAM) + LENGTH(KRAM);   /* top of RAM_KERN */
}
