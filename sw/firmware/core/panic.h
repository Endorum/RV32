#ifndef PANIC_H
#define PANIC_H

#include "trap.h"


void panic(trap_frame_t* tf, uint32_t cause, uint32_t tval);


#endif // PANIC_H