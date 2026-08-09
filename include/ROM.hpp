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
  explicit ROM() {
    std::ifstream f(T_config.firmware_path, std::ios::binary | std::ios::ate);
    if (!f)
      Error<std::runtime_error>("Could not open firmware file: '" +
                                T_config.firmware_path + "'");

    data.resize(static_cast<size_t>(f.tellg()));
    f.seekg(0);

    f.read(reinterpret_cast<char *>(data.data()), data.size());
  }

  u32 read(u32 address, BITSIZE size) override {
    if (address >= data.size()) {
      // rest is simply =0 so as to not run into an Error
      return 0x0;
    }

    u32 out = 0;
    for (int i = 0; i < size; i++) {
      out |= data.at(address + i) << (8 * i); // little-endian
    }

    if (T_config.B_debug)
      printf("ROM.read(address: %08X, size: %d) -> %08X out -> ", address, size,
             out);
    return out;
  }

  void write(u32 address, BITSIZE size, u32 value) override {
    // Error<std::invalid_argument>("ROM is read only!");
  }

  std::string str() const override {
    return "ROM with data of length: " + std::to_string(data.size()) +
           " Bytes ";
  }

private:
  std::vector<u8> data;
};

#endif // ROM_HPP
