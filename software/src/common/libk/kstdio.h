/*

Input output stuff used by the firmware before Kernel loaded

*/
#pragma once

#include "memory_map.h"

#include "stdarg.h"
#include "stdint.h"

// int print_dec(int v);
// int print_hex(unsigned v, int upper, int width);

int kvprintf(const char* fmt, va_list ap);
int kprintf(const char* fmt, ...);


int kvscanf(const char* fmt, va_list ap);
int kscanf(const char* fmt, ...);

uint32_t parse_hex(const char *s, int* bytes);