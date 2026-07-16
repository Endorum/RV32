#include "../include/CONFIG.hpp"
#include "../include/UTILS.hpp"

bool B_debug;
bool B_step;
Config T_config;
std::vector<uint32_t> V_u32_breakpoints;

void parseModules(std::string str) {
  // something like imf -> integer, multiply and divide, floating point ops

  for (auto c : str) {
    switch (c) {
    default:
      Error<std::invalid_argument>("Unknown configuration: '" +
                                   std::string(1, c) + "'");

    case 'a':
      T_config.A = true;
      break;
    case 'b':
      T_config.B = true;
      break;
    case 'c':
      T_config.C = true;
      break;
    case 'd':
      T_config.D = true;
      break;
    case 'f':
      T_config.F = true;
      break;
    case 'g':
      T_config.G = true;
      break;
    case 'h':
      T_config.H = true;
      break;
    case 'j':
      T_config.J = true;
      break;
    case 'l':
      T_config.L = true;
      break;
    case 'm':
      T_config.M = true;
      break;
    case 'n':
      T_config.N = true;
      break;
    case 'p':
      T_config.P = true;
      break;
    case 'q':
      T_config.Q = true;
      break;
    case 's':
      T_config.S = true;
      break;
    case 't':
      T_config.T = true;
      break;
    case 'v':
      T_config.V = true;
      break;
    }
  }
}
