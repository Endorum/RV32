#ifndef RAM_HPP
#define RAM_HPP

#include "CONFIG.hpp"
#include "DEFS.hpp"
#include "DEVICE.hpp"
#include "MAP.h"
#include "UTILS.hpp"
#include <chrono>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#define PAGE_SIZE 4096

constexpr u32 PAGE_AMOUNT = RAM_SIZE / PAGE_SIZE;

class RAM : public Device {
public:
  explicit RAM(u32 start, u32 size) : Device(start, size, "RAM") {}

  u32 load(u32 addr, BITSIZE size) override;

  void store(u32 addr, BITSIZE size, u32 value) override;

private:
  std::unordered_map<u32, std::unique_ptr<u8[]>> pages;
  u32 last_page = -1;
};

#endif // RAM_HPP
