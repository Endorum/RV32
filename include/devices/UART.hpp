#ifndef UART_HPP
#define UART_HPP

#include "DEVICE.hpp"
#include "CONFIG.hpp"
#include "DEFS.hpp"
#include "UTILS.hpp"

#define RBR_ADDR 0x00 // RO : Reciever Buffer Register, Read only
#define THR_ADDR 0x00 // WO : Transmitter Holding Register, Write only
#define IER_ADDR 0x01 // RW : Enable(1)/Disable(0) interrupts
#define IIR_ADDR 0x02 // RO : Information which int. occured, read only (default 0x01)
#define FCR_ADDR 0x02 // WO : Control behavior of the internal FIFOs. Write only
#define LCR_ADDR 0x03 // RW : LCR7 / DLAB is the only interesting one (?) (rw)
#define LSR_ADDR 0x05 // RO : Information about state of the uart, 0x60 (default) indicates its ready to transmit

#define RATE 4096

class UART : public Device {
public:
  explicit UART(u32 start, u32 size) : Device(start, size, "UART"){
    
  } 

  u32 load(u32 addr, u8 size) override;

  void store(u32 addr, u8 size, u32 value) override;

  void tick();

  void push_rx(u8 b);

private:
  std::deque<u8> rx;


  u8 IER = 0x00;
  u8 IIR = 0x01;
  u8 FCR = 0x00;
  u8 LCR = 0x00;

  u8 DLL = 0x00;
  u8 DLM = 0x00;

  
  u32 counter = 0;
  
};

#endif // UART_HPP