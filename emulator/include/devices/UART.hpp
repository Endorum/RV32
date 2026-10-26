#ifndef UART_HPP
#define UART_HPP

#include <deque>
#include <string>

#include "DEVICE.hpp"
#include "CONFIG.hpp"
#include "DEFS.hpp"
#include "UTILS.hpp"

#include "devices/uart16550.h"

#define RATE 4096

class UART : public Device {
public:
    explicit UART(u32 start, u32 size) : Device(start, size, "UART"){
    
    } 

    u32 load(u32 addr, u8 size) override;

    void store(u32 addr, u8 size, u32 value) override;

    void tick();

    void push_rx(u8 b);
  
    std::string take_tx();

private:
    std::deque<u8> rx;
    std::string tx;

    u8 IER = 0x00;
    u8 IIR = 0x01;
    u8 FCR = 0x00;
    u8 LCR = 0x00;

    u8 DLL = 0x00;
    u8 DLM = 0x00;

  
    u32 counter = 0;
  
};

#endif // UART_HPP