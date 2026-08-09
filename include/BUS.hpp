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

  void addDevice(const std::string &name, u32 start, u32 size, std::unique_ptr<Device> dev);

  u32 load(u32 address, BITSIZE size) const;

  void store(u32 address, BITSIZE size, u32 value);

  std::string str() const;

  std::string devices_string() const ;

private:
  std::vector<MemRegion> regions;
};

#endif // BUS_HPP
