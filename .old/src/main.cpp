#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include <stdio.h>

#include "csr.hpp"
#include "cpu.hpp"

u8* load_bin(const std::string& filename, size_t& out_size) {
    // Open file in binary mode
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        exit(-1);
        out_size = 0;
        return nullptr;
    }

    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Allocate buffer
    u8* buffer = new u8[size];

    // Read file into buffer
    if (!file.read(reinterpret_cast<char*>(buffer), size)) {
        std::cerr << "Failed to read file: " << filename << "\n";
        delete[] buffer;
        out_size = 0;
        return nullptr;
    }

    out_size = static_cast<size_t>(size);
    return buffer;
}

int main(){

    RV32 cpu;


    size_t size = 0;

    u8* rom = load_bin("rom.bin", size);
    
    BUS bus;

    cpu.attach_bus(&bus);
    cpu.bus->loadRom(rom, size);

    cpu.reset();

    cpu.step();

    cpu.debug();
    

    


    return 0;
}