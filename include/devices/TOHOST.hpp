#ifndef TOHOST_HPP
#define TOHOST_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

#include "DEVICE.hpp"
#include "CONFIG.hpp"
#include "DEFS.hpp"
#include "UTILS.hpp"

class TOHOST : public Device {
public:
  explicit TOHOST(u32 start, u32 size) : Device(start, size, "TOHOST"){}

  u32 load(u32 addr, BITSIZE size) override;

  void store(u32 addr, BITSIZE size, u32 value) override;

private:
};

#endif // TOHOST_HPP