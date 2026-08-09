#ifndef MMAP_HPP
#define MMAP_HPP

#include "DEVICE.hpp"

#include <cstdint>
#include <format>
#include <memory>
#include <string>

struct MemRegion {

  // 32 chars max
  std::string name;

  u32 start;
  u32 size;

  std::unique_ptr<Device> device;

  std::string str() const {
    return std::format("{:<16} start: {:08X} size: {:08X} device: {}", name,
                       start, size, static_cast<void *>(device.get()));
  }
};

#endif // MMAP_HPP
