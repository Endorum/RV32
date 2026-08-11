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
  std::cout << "TODO" << std::endl;
}



int main(int argc, char **argv) {

  // disabled forced args for testing
  if (argc < 1) {
    showUsage(argv[0]);
    return 1;
  } 


  // build configuration based on CLI arguments
  Config config = parse_arguments(argc, argv);
  std::cout << "Configuration: \n" << config.str() << std::endl;
  
  // setup
  Machine machine(config);
  machine.start();
  
  std::cout << "\nSTARTING EXECUTION...\n" << std::endl;

  while(1)
    machine.step();

  

  return 0;
}
