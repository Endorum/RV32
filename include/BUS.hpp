#ifndef BUS_HPP
#define BUS_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "DEVICE.hpp"
#include "MMAP.hpp"
#include "UTILS.hpp"

class Bus {

public:
  Bus() = default;
  ~Bus() = default;

  void addDevice(const std::string &name, uint32_t start, uint32_t size,
                 std::unique_ptr<Device> dev) {
    MemRegion reg = {name, start, size, std::move(dev)};

    std::cout << "Adding Device:\n" << reg.str() << std::endl;

    regions.push_back(std::move(reg));
  }

  uint8_t u8_read(uint32_t address) const {

    for (auto &r : regions) {
      if (r.start <= address && r.start + r.size > address)
        return r.device->u8_read(address);
    }

    Error<std::out_of_range>(
        std::format("Address: {:08X} not mapped", address));

    return -1;
  }

  void u8_write(uint32_t address, uint8_t value) {

    for (auto &r : regions) {
      if (r.start <= address && r.start + r.size > address)
        r.device->u8_write(address, value);
    }

    Error<std::out_of_range>(
        std::format("Address: {:08X} not mapped", address));
  }

  std::string str() const {
    std::string out = "";
    for (auto &reg : regions) {
      out += reg.str() + "\n";
    }
    return out;
  }

private:
  std::vector<MemRegion> regions;
};

#endif // BUS_HPP
