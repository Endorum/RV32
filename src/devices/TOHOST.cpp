#include "TOHOST.hpp"

u32 TOHOST::load(u32 addr, BITSIZE size) {
  return u32();
}

void TOHOST::store(u32 addr, BITSIZE size, u32 value) {
  bool test = value & 0x80000000;
  if(test){
    std::cout << std::format("TOHOST FAIL WITH CODE {}", value >> 1);
    
    exit(value >> 1);
  }else{
    std::cout << "PASS" << std::endl;
  }
}
