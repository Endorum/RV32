#include "screen.h"
#include "memory_map.h"
#include "devices/screenInfo.h"
#include <stdint.h>

void drawPixel(uint8_t x, uint8_t y, uint8_t color){
    

    uint32_t addr = (x * SCREEN_WIDTH + y) + SCREEN_START;
    *(volatile uint8_t*)(uintptr_t)addr = color;
    
}


