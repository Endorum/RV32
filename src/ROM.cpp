#include "ROM.hpp"


u32 ROM::load(u32 address, BITSIZE size) {
  if (address >= data.size()) {
    // rest is simply = 0 so as to not run into an Error
    // TODO: raise an error?
    return 0x0;
  }

  u32 out = 0;
  for (int i = 0; i < size; i++) {
    out |= data.at(address + i) << (8 * i); // little-endian
  }

  return out;
}
