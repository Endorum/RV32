#include "devices/SCREEN.hpp"
#include "DEFS.hpp"
#include "ERROR.hpp"

u32 SCREEN::load(u32 addr, u8 size) {
    if (addr >= SCREEN_WIDTH * SCREEN_HEIGHT)
        throw TrapException(EXCEPTION_CODE::LOAD_ACCESS_FAULT, addr);
    
    return data[addr];
}

void SCREEN::store(u32 addr, u8 size, u32 value) {
    if (addr >= SCREEN_WIDTH * SCREEN_HEIGHT)
        throw TrapException(EXCEPTION_CODE::STORE_ACCESS_FAULT, addr);
    
    data[addr] = value;
}

void SCREEN::tick() {
    
}

void SCREEN::drawPixel(u8 x, u8 y, u8 color){
    if(x >= SCREEN_WIDTH) return;
    if(y >= SCREEN_HEIGHT) return;

    u32 addr = y * SCREEN_WIDTH + x;
    store(addr, BYTE, color); // rr gg bb 00
}

const std::vector<u8>& SCREEN::getScreen() const {
    return data;
}


