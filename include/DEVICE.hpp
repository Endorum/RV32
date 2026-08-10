#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "DEFS.hpp"
#include <cstdint>
#include <string>

// a device is a class of anything that can be attached to the bus and
// can act like a memory device, in the sense that it can simply be accessed via
// memory read/write ops
// a Device is abstract and can therefor not be instantiatet.
// a Device now also holds its memory start and size and name
class Device {
public:
  Device(u32 st, u32 sz, const std::string& n) : start(st), mem_size(sz), name(n) {}

  virtual ~Device() = default;

  virtual u32 load(u32 address, BITSIZE size) = 0; 
  virtual void store(u32 address, BITSIZE size, u32 value) = 0;
  

  u32 get_start() const { return start; }
  u32 get_size() const { return mem_size; }

  Config get_config() const { return config; }
  void set_config(const Config& c) { config = c; }

  std::string str() const {return std::format("'{}' start: {:08X} size: {} Bytes", name, start, mem_size);}

private:
  u32 start;
  u32 mem_size;
  std::string name;
  Config config;
};

#endif // DEVICE_HPP
