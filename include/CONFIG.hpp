#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "DEFS.hpp"

inline std::string breakpoints_str(const std::vector<u32> &bp) {
  std::string out = " [";

  for (auto v : bp) {
    out += std::format("0x{:x}, ", v);
  }

  // remove last ,
  if (!bp.empty())
    out = out.substr(0, out.length() - 2);

  return out + "]";
}

struct Config {
  bool A = false; /* Atomic instructions */
  bool B = false; /* Bit manipulation */
  bool C = false; /* Compressed instructions */
  bool D = false; /* Double-precision floating-point */
  bool F = false; /* Single-precision floating-point */
  bool G = false; /* Shorthand for IMAFD extensions */
  bool H = false; /* Hypervisor extension */
  bool J = false; /* Dynamically translated languages */
  bool L = false; /* Decimal floating-point */
  bool M = false; /* Integer multiplication and division */
  bool N = false; /* User-level interrupts */
  bool P = false; /* Packed-SIMD instructions */
  bool Q = false; /* Quad-precision floating-point */
  bool S = false; /* Supervisor mode */
  bool T = false; /* Transactional memory */
  bool V = false; /* Vector operations */

  std::string str() {
    return " A: " + std::to_string(A) + " B: " + std::to_string(B) + "\n" +
           " C: " + std::to_string(C) + " D: " + std::to_string(D) + "\n" +
           " F: " + std::to_string(F) + " G: " + std::to_string(G) + "\n" +
           " H: " + std::to_string(H) + " J: " + std::to_string(J) + "\n" +
           " L: " + std::to_string(L) + " M: " + std::to_string(M) + "\n" +
           " N: " + std::to_string(N) + " P: " + std::to_string(P) + "\n" +
           " Q: " + std::to_string(Q) + " S: " + std::to_string(S) + "\n" +
           " T: " + std::to_string(T) + " V: " + std::to_string(V) + "\n" +
           " DEBUG: " + std::to_string(B_debug) +
           " STEP: " + std::to_string(B_step) + "\n" + " breakpoints: \n" +
           breakpoints_str(V_u32_breakpoints) + "\n" +
           " Firmware: " + firmware_path + "\n Harddisk: " + harddisk_path +
           "\n Removable: " + removable_path + "\n";
  }
  bool B_debug = false;
  bool B_step = false;

  std::vector<u32> V_u32_breakpoints;

  std::string firmware_path = "";
  std::string harddisk_path = "";
  std::string removable_path = "";

  u32 resetVector = 0x0;
};

struct CLIArgument {

  // in
  std::string S_type;
  std::string S_value;
};

void parseArgument(CLIArgument arg);

extern Config T_config;

void parseModules(std::string str);

#endif // CONFIG_HPP
