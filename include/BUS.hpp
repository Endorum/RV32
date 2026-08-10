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
#include "UTILS.hpp"

class BUS {

public:
  BUS() = default;
  ~BUS() = default;

  void addDevice(Device& dev);

  u32 load(u32 address, BITSIZE size) const;

  void store(u32 address, BITSIZE size, u32 value);

  std::string str() const;

private:
  std::vector<Device*> devices;
};

#endif // BUS_HPP
