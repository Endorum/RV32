#ifndef ROM_HPP
#define ROM_HPP

#include "CONFIG.hpp"
#include "DEVICE.hpp"
#include "UTILS.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <stdexcept>

class ROM : public Device {

public:
  explicit ROM() {
    std::ifstream f(T_config.firmware_path, std::ios::binary | std::ios::ate);
    if (!f)
      Error<std::runtime_error>("Could not open firmware file: '" +
                                T_config.firmware_path + "'");

    data.resize(static_cast<size_t>(f.tellg()));
    f.seekg(0);

    f.read(reinterpret_cast<char *>(data.data()), data.size());
  }

  uint8_t u8_read(uint32_t address) const override {
    // TODO: somehow retrieve the device size, to check for valid addresses?

    return data.at(address);
  }

  void u8_write(uint32_t address, uint8_t value) override {
    Error<std::invalid_argument>("ROM is read only!");
  }

private:
  bool loaded = false;

  std::vector<uint8_t> data;
};

#endif // ROM_HPP
