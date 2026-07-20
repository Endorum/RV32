#include "../include/CONFIG.hpp"
#include "../include/UTILS.hpp"

Config T_config;

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

void parseArgument(CLIArgument arg) {

  if (arg.S_type == "m") {
    parseModules(arg.S_value);
  }

  else if (arg.S_type == "d") {
    T_config.B_debug = S_to_B(arg.S_value);
  }

  else if (arg.S_type == "s") {
    T_config.B_step = S_to_B(arg.S_value);
  } else if (arg.S_type == "b") {

    uint32_t bp = 0;
    try {
      bp = std::strtoul(arg.S_value.c_str(), nullptr, 16);
    } catch (std::exception E) {
      Error<std::invalid_argument>("Invalid breakpoint address");
    }

    T_config.V_u32_breakpoints.push_back(bp);

  }

  else if (arg.S_type == "firmware") {
    T_config.firmware_path = arg.S_value;
  } else if (arg.S_type == "disk") {
    T_config.harddisk_path = arg.S_value;
  } else if (arg.S_type == "removable") {
    T_config.removable_path = arg.S_value;
  } else {
    Error<std::invalid_argument>("commandline option'" + arg.S_type +
                                 "' not supported! ");
  }
}
