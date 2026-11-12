#pragma once
#include <stdint.h>

#define SECTOR_SIZE 512

// buf size is 512
void hd_read_sector(uint32_t n, uint8_t* buf);
void hd_write_sector(uint32_t n, uint8_t* buf);