#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <cstdint>

// a device is a class of anything that can be attached to the bus and
// can act like a memory device, in the sense that it can simply be accessed via
// memory read/write ops
// a Device is abstract and can therefor not be instantiatet.
class Device {
public:
  virtual ~Device() = default;

  virtual uint8_t u8_read(uint32_t address) const = 0;
  virtual void u8_write(uint32_t address, uint8_t value) = 0;
};

#endif // DEVICE_HPP
