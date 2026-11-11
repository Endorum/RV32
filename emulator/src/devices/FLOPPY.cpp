#include "devices/FLOPPY.hpp"
#include "DEVICE.hpp"
#include "UTILS.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>

u32 FLOPPY::load(u32 addr, u8 size) {
    delay();
    
    u8 buf[4] = {};
    file.seekg(addr);
    
    file.read(reinterpret_cast<char*>(buf), (u8)(size));
    file.clear();
    
    u32 value = 0;
    for(u32 i=0;i<size;i++){
        value |= static_cast<u32>(buf[i]) << (8 * i);
    }
    
    
    return value;
}

void FLOPPY::store(u32 addr, u8 size, u32 value){
    delay();
    
    u8 buf[4] = {};
    
    for(u32 i=0;i<size;i++){
        buf[i] = (value >> (8 * i)) & 0xFF;
    }
    
    file.seekp(addr);
    file.write(reinterpret_cast<char*>(buf), (u8)size);
    file.flush();
    
}

void FLOPPY::insertDisk(std::string filename) {
    std::cout << "Loading file '" + filename + "' to floppy disk: '" + get_name() + "'";
    
    file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
    
    if(!file.is_open()){
        Error<std::runtime_error>("Could not open floppy disk file: '" + filename + "'");
    }
    
    setup = true;
    std::cout << "Done!" << std::endl;
}

void FLOPPY::delay() {
    std::this_thread::sleep_for(std::chrono::microseconds(delay_mus));
}
