#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <string>
#include <vector>

extern bool B_debug;
extern bool B_step;
extern std::vector<uint32_t> V_u32_breakpoints;

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
    return " A: " + std::to_string(A) + " B: " + std::to_string(B) +
           " C: " + std::to_string(C) + " D: " + std::to_string(D) +
           " F: " + std::to_string(F) + " G: " + std::to_string(G) +
           " H: " + std::to_string(H) + " J: " + std::to_string(J) +
           " L: " + std::to_string(L) + " M: " + std::to_string(M) +
           " N: " + std::to_string(N) + " P: " + std::to_string(P) +
           " Q: " + std::to_string(Q) + " S: " + std::to_string(S) +
           " T: " + std::to_string(T) + " V: " + std::to_string(V);
  }
};

extern Config T_config;

void parseModules(std::string str);

#endif // CONFIG_HPP
