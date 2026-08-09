#ifndef UTILS_HPP
#define UTILS_HPP

#include <fstream>
#include <iostream>
#include <string>

#include "DEFS.hpp"

template <typename ExceptionType> [[noreturn]] void Error(std::string msg);

bool S_to_B(std::string str);
u32 extract_bits(u32 value, u32 min, u32 max);
i32 sign_extend(u32 value, int bits);

std::string mem_str(u8 *mem, u32 start, u32 size, u8 cols = 8);
std::string reg_idx_str(u32 idx);
#endif // UTILS_HPP
