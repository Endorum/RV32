#ifndef BUS_HPP
#define BUS_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "CONFIG.hpp"
#include "DEVICE.hpp"
#include "MMAP.hpp"
#include "UTILS.hpp"

class Bus {

public:
  Bus() = default;
  ~Bus() = default;

  void addDevice(const std::string &name, u32 start, u32 size,
                 std::unique_ptr<Device> dev) {
    MemRegion reg = {name, start, size, std::move(dev)};

    std::cout << "Adding Device:\n" << reg.str() << std::endl;

    regions.push_back(std::move(reg));
  }

  u32 read(u32 address, BITSIZE size) const {

    if (T_config.B_debug)
      printf("Bus.read(address: %08X, size: %d): ", address, size);

    for (auto &r : regions) {
      if (r.start <= address && r.start + r.size > (address + size)) {
        return r.device->read(address - r.start, size);
      }
    }

    Error<std::out_of_range>(
        std::format("Address: {:08X} not mapped", address));

    return -1;
  }

  void write(u32 address, BITSIZE size, u32 value) {

    for (auto &r : regions) {
      if (r.start <= address && r.start + r.size > (address + size)) {
        r.device->write(address - r.start, size, value);
        return;
      }
    }

    Error<std::out_of_range>(
        std::format("Address: {:08X} not mapped", address));
  }

  std::string str() const {
    std::string out =
        "BUS with " + std::to_string(regions.size()) + " attached devices:\n";
    for (auto &reg : regions) {
      out += reg.str() + "\n";
    }
    return out;
  }

  std::string devices_string() const {
    std::string out = "";
    for (auto &reg : regions) {
      out += reg.device->str() + "\n";
    }
    return out;
  }

private:
  std::vector<MemRegion> regions;
};

#endif // BUS_HPP
