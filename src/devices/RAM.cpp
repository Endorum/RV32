#include "RAM.hpp"

u32 RAM::load(u32 addr, BITSIZE size) {

  u32 value = 0;

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

    value |= static_cast<u32>(page[page_off] << (8 * i));
    last_page = page_num;
  }

  return value;
}

void RAM::store(u32 addr, BITSIZE size, u32 value) {

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