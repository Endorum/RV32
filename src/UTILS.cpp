#include <algorithm>
#include <format>
#include <stdexcept>

#include "../include/UTILS.hpp"

template <typename ExceptionType> [[noreturn]] void Error(std::string msg) {

  ExceptionType exc(msg);

  std::cerr << "\tERROR: " << exc.what() << "\n\t";

  throw exc;
}

// The template body lives in this .cpp, so every ExceptionType used from
// other translation units must be explicitly instantiated here — otherwise
// the linker has no symbol to find. (Alternative: move the definition into
// UTILS.hpp and delete these.)
template void Error<std::runtime_error>(std::string);
template void Error<std::invalid_argument>(std::string);
template void Error<std::out_of_range>(std::string);

bool S_to_B(std::string str) {

  std::transform(str.begin(), str.end(), str.begin(), ::toupper);

  if (str == "TRUE")
    return true;
  if (str == "FALSE")
    return false;

  Error<std::invalid_argument>("Could not convert '" + str + "' to boolean");
}

u32 extract_bits(u32 value, u32 min, u32 max) {

  if (min > max) {
    printf("Error: min > max\n");
    exit(1);
  }

  u32 width = max - min + 1; // number of bits
  u32 mask = (1u << width) - 1;

  return (value >> min) & mask;
}

i32 sign_extend(u32 value, int bits) {
  i32 mask = 1u << (bits - 1); // the sign bit
  return (value ^ mask) - mask;
}

std::string mem_str(u8 *mem, u32 start, u32 size, u8 cols) {
  std::string out = "";

  for (u32 off = 0; off <= size; off++) {

    if (off % cols == 0)
      out += "\n";

    if (off % cols == 0) {
      out += std::format("{:08X}: ", start + off);
    }
    out += std::format("{:02X} ", mem[start + off]);
  }

  return out;
}

std::string reg_idx_str(u32 idx) {
  switch (idx) {
  default:
    return "Unknown register index: " + std::to_string(idx);
  case zero:
    return "zero";
  case ra:
    return "ra";
  case sp:
    return "sp";
  case gp:
    return "gp";
  case tp:
    return "tp";
  case t0:
    return "t0";
  case t1:
    return "t1";
  case t2:
    return "t2";
  case s0:
    return "s0";
  case s1:
    return "s1";
  case a0:
    return "a0";
  case a1:
    return "a1";
  case a2:
    return "a2";
  case a3:
    return "a3";
  case a4:
    return "a4";
  case a5:
    return "a5";
  case a6:
    return "a6";
  case a7:
    return "a7";
  case s2:
    return "s2";
  case s3:
    return "s3";
  case s4:
    return "s4";
  case s5:
    return "s5";
  case s6:
    return "s6";
  case s7:
    return "s7";
  case s8:
    return "s8";
  case s9:
    return "s9";
  case s10:
    return "s10";
  case s11:
    return "s11";
  case t3:
    return "t3";
  case t4:
    return "t4";
  case t5:
    return "t5";
  case t6:
    return "t6";
  }

  return "";
}
