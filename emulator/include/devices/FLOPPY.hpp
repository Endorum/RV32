#pragma once

#include "DEVICE.hpp"
#include <cstdint>
#include <fstream>

class FLOPPY : public Device{
public:
    explicit FLOPPY(u32 start, u32 size, const char letter) : Device(start, size, "FLOPPY_" + std::string(1, letter)){}
    
    u32 load(u32 addr, u8 size) override;
    void store(u32 addr, u8 size, u32 value) override;
    
    void insertDisk(std::string filename);
    
private:

    bool present = false;
    uint32_t diskSize = 0; // cant load past here
    std::fstream file;
    
    bool setup = false;

    u32 delay_mus = 100;

    void delay();

};

