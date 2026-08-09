#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "DEFS.hpp"
#include <cstdint>
#include <string>

// a device is a class of anything that can be attached to the bus and
// can act like a memory device, in the sense that it can simply be accessed via
// memory read/write ops
// a Device is abstract and can therefor not be instantiatet.
class Device {
public:
  virtual ~Device() = default;

  virtual u32 read(u32 address, BITSIZE size) = 0; 
  virtual void write(u32 address, BITSIZE size, u32 value) = 0;
  virtual std::string str() const = 0;
};

#endif // DEVICE_HPP
