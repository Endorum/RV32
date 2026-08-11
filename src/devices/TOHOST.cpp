#include "TOHOST.hpp"

u32 TOHOST::load(u32 addr, BITSIZE size) {
  return u32();
}

void TOHOST::store(u32 addr, BITSIZE size, u32 value) {
  if((value & 0x1) == 0) return; // kein exit request

  if(value == 1){
    std::cout << "PASS" << std::endl;
    exit(0);
  }else{
    std::cout << std::format("TOHOST FAIL WITH CODE {}", value >> 1);
    exit(1);
  }
}
