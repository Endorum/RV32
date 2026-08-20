#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "DEFS.hpp"
#include "MAP.hpp"

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
  u32 tohost_address = 0x0;

  u32 ram_start = RAM_START;
  u32 ram_size = RAM_SIZE;
  u32 rom_start = ROM_START;
  u32 rom_size = ROM_SIZE;
  u32 hd_start = HD_START;
  u32 hd_size = HD_SIZE;

  u32 clint_start = CLINT_START;
  u32 clint_size = CLINT_SIZE;

  u32 uart_start = UART_START;
  u32 uart_size = UART_SIZE;
};

Config parse_arguments(int argc, char** argv);
std::string breakpoints_str(const std::vector<u32> &bp);

#endif // CONFIG_HPP
