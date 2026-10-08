#include "CLINT.hpp"

u32 CLINT::load(u32 addr, u8 size) {
  if(size != WORD) return 0;

  if(addr == MSIP_ADDR) return msip;

  if(addr == MTIMECMP_ADDR) return (u32)(mtimecmp); // low
  if(addr == MTIMECMP_ADDR + 4) return (u32)(mtimecmp >> 32); // high
  
  if(addr == MTIME_ADDR) return (u32)(mtime); // low
  if(addr == MTIME_ADDR + 4) return (u32)(mtime >> 32); // high

  return 0;
}

void CLINT::store(u32 addr, u8 size, u32 value) {
  if(size != WORD) return;
  
  if(addr == MSIP_ADDR) 
    msip = value & 1;

  else if(addr == MTIMECMP_ADDR)      mtimecmp = (mtimecmp & 0xFFFFFFFF00000000) | (value & 0x00000000FFFFFFFF); // low
  else if(addr == MTIMECMP_ADDR + 4)  mtimecmp = (mtimecmp & 0x00000000FFFFFFFF) | ((((u64)value) << 32) & 0xFFFFFFFF00000000); // high
  
  else if(addr == MTIME_ADDR)     mtime = (mtime & 0xFFFFFFFF00000000) | (value & 0x00000000FFFFFFFF); // low
  else if(addr == MTIME_ADDR + 4) mtime = (mtime & 0x00000000FFFFFFFF) | ((((u64)value) << 32) & 0xFFFFFFFF00000000); // high

  return;
}

void CLINT::tick() {
  mtime++;
}
