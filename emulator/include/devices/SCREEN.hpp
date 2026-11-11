#pragma once

#include "DEFS.hpp"
#include "DEVICE.hpp"
#include <vector>
 
#include "devices/screenInfo.h"



class SCREEN : public Device{
public:
    
    explicit SCREEN(u32 start, u32 end) : Device(start, end, "SCREEN"){
        data.resize(SCREEN_WIDTH * SCREEN_HEIGHT);
    }
    
    u32 load(u32 addr, u8 size) override;
    void store(u32 addr, u8 size, u32 value) override;
    void tick();
    

    void drawPixel(u8 x, u8 y, u8 color);
    const std::vector<u8>& getScreen() const;
    
    
    
private:
    // 1 byte = 1 px
    std::vector<u8> data;

    
};