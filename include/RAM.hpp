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
  explicit RAM() {}

  u32 read(u32 addr, BITSIZE size) override {
    if (T_config.B_debug)
      printf("RAM.read(%08X, %d) -> ", addr, size);

    u32 value = 0;

    for (u32 i = 0; i < size; i++) {
      u32 address = addr + i;
      u32 page_num = address / PAGE_SIZE;
      u32 page_off = address % PAGE_SIZE;

      if (page_num >= PAGE_AMOUNT) {
        Error<std::runtime_error>(std::format(
            "Memory access out of range, page number: {}\n", page_num));
      }

      auto &page = pages[page_num];

      // create a new page if not found
      if (!page) {
        page = std::make_unique<u8[]>(PAGE_SIZE);
      }

      value |= static_cast<u32>(page[page_off] << (8 * i));
      last_page = page_num;
    }

    if (T_config.B_debug)
      printf("%08X\n", value);

    return value;
  }

  void write(u32 addr, BITSIZE size, u32 value) override {
    if (T_config.B_debug)
      printf("RAM.write(%08X, %d, %08X)", addr, size, value);

    for (u32 i = 0; i < size; i++) {
      u32 address = addr + i;
      u32 page_num = address / PAGE_SIZE;
      u32 page_off = address % PAGE_SIZE;

      if (page_num >= PAGE_AMOUNT) {
        Error<std::runtime_error>(
            std::format("Memory access out of range, page number: {}, max page "
                        "amount: {}\n",
                        page_num, PAGE_AMOUNT));
      }

      auto &page = pages[page_num];

      // create a new page if not found
      if (!page) {
        page = std::make_unique<u8[]>(PAGE_SIZE);
      }

      page[page_off] = (value >> (8 * i)) & 0xFF;
      last_page = page_num;
    }
  }

  std::string str() const override {

    int idx = last_page;

    std::string s;
    if (auto it = pages.find(idx); it != pages.end()) {
      s = mem_str(it->second.get(), 0, 0xFF);
    }

    return std::format(
               "RAM of total size {} Bytes, and currently {} allocated pages",
               RAM_SIZE, pages.size()) +
           "\nLast Page:\n" + s;
  }

private:
  std::unordered_map<u32, std::unique_ptr<u8[]>> pages;
  u32 last_page = -1;
};

#endif // RAM_HPP
