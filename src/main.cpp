#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/CONFIG.hpp"
#include "../include/UTILS.hpp"

void showUsage(std::string argv0) {
  std::cout << "Usage:\n" << std::endl;
  std::cout << argv0 << "   [-mdsb] [options] " << std::endl;
  std::cout << "-m=[amf...]       : extensions | specify one or more modules, "
               "currenty available: i"
            << std::endl;
  std::cout
      << "-d=[true/false]   : debug mode | enables or disables Debug mode "
      << std::endl;
  std::cout << "-s=[true/false]   : step mode  | enabled stepping, instruction "
               "per instruction, (enables debug mode by default?)"
            << std::endl;
  std::cout
      << "-b=0x...          : breakpoint | places breakpoint at given address "
         "in memory, stops and enables step mode by default and debug mode"
      << std::endl;
  std::cout << "-ffw=.../.../.bin : firmware   | provides a firmware file "
               "which is run at reset by default"
            << std::endl;
  std::cout << "-fhd=.../.../.bin : harddisk   | to provide a file which is "
               "used as permanent storage/Harddisk"
            << std::endl;
  std::cout << "-ffd=.../.../.bin : floppydisk | path to a file which can be "
               "loaded for Programs or the OS"
            << std::endl; // TODO: multiple files / multiple floppies? hot swap?
  std::cout << "-h                : help       | shows the usage" << std::endl;
}

struct CLIArgument {

  // in
  std::string S_type;
  std::string S_value;
};

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

  else if (arg.S_type[0] == 'f') {
    std::string filename = arg.S_value;
    if (arg.S_type == "ffw") {
      T_config.firmware_path = filename;
    } else if (arg.S_type == "fhd") {
      T_config.harddisk_path = filename;
    } else if (arg.S_type == "ffd") {
      T_config.floppydisk_path = filename;
    }
  }

  else {
    Error<std::invalid_argument>("commandline option'" + arg.S_type +
                                 "' not supported! ");
  }
}

int main(int argc, char **argv) {

  if (argc < 2) {
    showUsage(argv[0]);
  }

  for (int i = 1; i < argc; i++) {
    std::string S_arg(argv[i]);
    S_arg = S_arg.substr(1, S_arg.length());

    size_t pos = S_arg.find("=");

    if (pos != std::string::npos) {
      std::string S_type = S_arg.substr(0, pos);
      std::string S_value = S_arg.substr(pos + 1);

      CLIArgument arg = {S_type, S_value};

      parseArgument(arg);

    } else {

      Error<std::invalid_argument>("Invalid argument near: '" +
                                   std::string(argv[i]) + "'");
    }
  }

  std::cout << "Configuration: \n" << T_config.str() << std::endl;

  return 0;
}
