#pragma once

#include "stdint.h"


uint8_t read_byte(uint32_t addr);
uint32_t read_word(uint32_t addr);

void    write_byte(uint32_t addr, uint8_t value);
void write_word(uint32_t addr, uint32_t word);