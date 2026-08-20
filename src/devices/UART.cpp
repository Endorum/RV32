#include "UART.hpp"

#include <poll.h>
#include <unistd.h>

u32 UART::load(u32 addr, u8 size) {
  if(size != BYTE) return 0;

  if(addr == RBR_ADDR) {

    if(LCR & 0x80){ // DLAB=1
      return DLL;
    }

    if(rx.empty()) return 0x00;
    
    u32 ret = rx.front();
    rx.pop_front();
    
    return ret;
  }

  if(addr == IER_ADDR) {
    
    if(LCR & 0x80){ // DLAB=1
      return DLM;
    }

    return IER;
  }

  if(addr == IIR_ADDR) return IIR;
  
  if(addr == LCR_ADDR) return LCR;
  
  if(addr == LSR_ADDR){
    return 0x60 | (rx.empty() ? 0 : 1); 
  }

  return 0x0;
}

void UART::store(u32 addr, u8 size, u32 value) {
  if(size != BYTE) return;

  if(addr == THR_ADDR) {
    
    if(LCR & 0x80){ // DLAB=1
      DLL = value;
      return;
    }

    std::cout.put(char(value));
    std::cout.flush();
  }

  if(addr == IER_ADDR) { 
    if(LCR & 0x80) { // DLAB=1
      DLM = value;
      return;
    }
    IER = value; 
    return; 
  }
  
  if(addr == FCR_ADDR) {
    // empty rx
    if(value & 0x2){
      rx.erase(rx.begin(), rx.end());
    } 
    if(value & 0x4){
      // empty fx queue, ie do nothing
    }
    FCR = value;
  }

  if(addr == LCR_ADDR) { LCR = value; return; }

  return;
}

void UART::tick() {

  // only poll every RATEnth tick
  if(counter >= RATE){
    struct pollfd pfd = { .fd = 0, .events = POLLIN }; // fd = 0 = stdin
    if(poll(&pfd, 1, /*timeout_ms=*/0) > 0){
      u8 b;
      if (read(STDIN_FILENO, &b, 1) == 1) // read into a single byte
        push_rx(b);
    }

    counter = 0;
  }
  
  counter++;
}

void UART::push_rx(u8 b) {
  rx.push_back(b);
}
