#ifndef ROM_HPP
#define ROM_HPP

#include "CONFIG.hpp"
#include "DEVICE.hpp"
#include "UTILS.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

class ROM : public Device {

public:
  explicit ROM(u32 start, u32 size) : Device(start, size, "ROM") {}

  void load_firmware();

  u32 load(u32 address, u8 size) override;

  void store(u32 address, u8 size, u32 value) override {
    // Error<std::invalid_argument>("ROM is read only!");
    // raise error?
  }

private:
  std::vector<u8> data;
};

#endif // ROM_HPP
