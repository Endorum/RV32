#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "DEFS.hpp"

std::string breakpoints_str(const std::vector<u32> &bp);

struct Config {
  // bool A = false; /* Atomic instructions */
  // bool B = false; /* Bit manipulation */
  // bool C = false; /* Compressed instructions */
  // bool D = false; /* Double-precision floating-point */
  // bool F = false; /* Single-precision floating-point */
  // bool G = false; /* Shorthand for IMAFD extensions */
  // bool H = false; /* Hypervisor extension */
  // bool J = false; /* Dynamically translated languages */
  // bool L = false; /* Decimal floating-point */
  // bool M = false; /* Integer multiplication and division */
  // bool N = false; /* User-level interrupts */
  // bool P = false; /* Packed-SIMD instructions */
  // bool Q = false; /* Quad-precision floating-point */
  // bool S = false; /* Supervisor mode */
  // bool T = false; /* Transactional memory */
  // bool V = false; /* Vector operations */

  std::string str() {
    return " DEBUG: " + std::to_string(debug) +
           " LOG: " + std::to_string(log) +
           " STEP: " + std::to_string(step) + "\n" + " breakpoints: \n" +
           breakpoints_str(breakpoints) + "\n" +
           " Firmware: " + firmware_path + "\n Harddisk: " + harddisk_path +
           "\n Removable: " + removable_path + "\n";
  }

  bool debug = false;
  bool log = false;
  bool step = false;

  std::vector<u32> breakpoints;

  std::string firmware_path = "";
  std::string harddisk_path = "";
  std::string removable_path = "";

  u32 reset_vector = 0x0;
};

struct CLIArgument {
  std::string S_type;
  std::string S_value;
};

void parseArgument(CLIArgument arg, Config& config);

#endif // CONFIG_HPP
