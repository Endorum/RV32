#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "BUS.hpp"
#include "CONFIG.hpp"
#include "CPU.hpp"
#include "MACHINE.hpp"
#include "ROM.hpp"
#include "UTILS.hpp"

void showUsage(std::string argv0) {
  std::cout << "Usage:\n" << std::endl;
  std::cout << argv0 << "   [-dsb] [options] " << std::endl;
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
  std::cout << "-firmware=.../.../.bin : firmware   | provides a firmware file "
               "which is run at reset by default"
            << std::endl;
  std::cout << "-disk=.../.../.bin : harddisk   | to provide a file which is "
               "used as permanent storage/Harddisk"
            << std::endl;
  std::cout
      << "-removable=.../.../.bin : removable | path to a file which can be "
         "loaded for Programs or the OS"
      << std::endl; // TODO: multiple files / multiple floppies? hot swap?
  std::cout << "-h                : help       | shows the usage" << std::endl;
}

int main(int argc, char **argv) {

  // disabled forced args for testing
  if (argc < 1) {
    showUsage(argv[0]);
    return 1;
  } 


  // build configuration based on CLI arguments
  Config config;
  for (int i = 1; i < argc; i++) {
    std::string S_arg(argv[i]);
    S_arg = S_arg.substr(1, S_arg.length());

    size_t pos = S_arg.find("=");

    if (pos != std::string::npos) {
      std::string S_type = S_arg.substr(0, pos);
      std::string S_value = S_arg.substr(pos + 1);

      CLIArgument arg = {S_type, S_value};

      parseArgument(arg, config);

    } else {

      Error<std::invalid_argument>("Invalid argument near: '" +
                                   std::string(argv[i]) + "'");
    }
  }
  std::cout << "Configuration: \n" << config.str() << std::endl;

  // setup
  Machine machine(config);
  machine.start();
  
  std::cout << "\nSTARTING EXECUTION...\n" << std::endl;

  for(int i=0;i<100;i++)
    machine.step();
  

    

  return 0;
}
