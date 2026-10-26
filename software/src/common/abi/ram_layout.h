// How the software splits the RAM between the stages (firmware <-> kernel).
// Pure software convention -- the hardware (emulator) does not know about it.
// Must stay #define-only: it is also included by the linker scripts.

#ifndef RAM_LAYOUT_H
#define RAM_LAYOUT_H

#include "../../../platform/memory_map.h"

#define RAM_FIRM_START  RAM_START
#define RAM_FIRM_SIZE   0x40000

#define RAM_FREE_START  (RAM_FIRM_START + RAM_FIRM_SIZE)
#define RAM_FREE_SIZE   0x10000

#define RAM_KERN_START  (RAM_FREE_START + RAM_FREE_SIZE)
#define RAM_KERN_SIZE   0x100000

#define RAM_USER_START  (RAM_KERN_START + RAM_KERN_SIZE)
#define RAM_USER_SIZE   ((RAM_START + RAM_SIZE) - RAM_USER_START)

#endif // RAM_LAYOUT_H
