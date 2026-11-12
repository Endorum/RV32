#include "hd.h"
#include "memory_map.h"
#include "stdint.h"
#include "kstring.h"
#include "mem.h"

void hd_read_sector(uint32_t n, uint8_t* buf){
   
    uint32_t src = HD_START + n * SECTOR_SIZE;
    
    for(uint32_t i = 0 ; i < SECTOR_SIZE; i++){
        buf[i] = read_byte(src + i);
    }
    
}

void hd_write_sector(uint32_t n, uint8_t *buf){
    uint32_t dst = HD_START + n * SECTOR_SIZE;
    
    for(uint32_t i = 0; i < SECTOR_SIZE; i++){
        write_byte(dst + i, buf[i]);
    }
}


