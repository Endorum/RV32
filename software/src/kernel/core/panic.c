#include "panic.h"

void panic() {
    asm volatile("ebreak");
}