#ifndef HD_HPP
#define HD_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

#include "DEVICE.hpp"
#include "CONFIG.hpp"
#include "DEFS.hpp"
#include "UTILS.hpp"

class HD : public Device {
public:
  explicit HD(u32 start, u32 size) : Device(start, size, "HD"){}

  u32 load(u32 addr, u8 size) override;

  void store(u32 addr, u8 size, u32 value) override;  

  void setup_harddisk();

private:
  std::string filepath;
  std::fstream file;

  u32 delay_mus = 100;
  void delay();
};

#endif // HD_HPP