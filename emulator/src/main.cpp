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
#include "devices/ROM.hpp"
#include "UTILS.hpp"

void showUsage(std::string argv0) {
  std::cout
      << "Usage: " << argv0 << " [options]\n"
      << "\n"
      << "Flags:\n"
      << "  -d                    enable debug output\n"
      << "  -l                    enable logging\n"
      << "  -s                    single-step execution\n"
      << "\n"
      << "Files:\n"
      << "  -firmware=<path>      firmware image loaded into ROM\n"
      << "  -harddisk=<path>      harddisk image\n"
      << "  -removeable=<path>    removable media image\n"
      << "\n"
      << "Addresses / sizes (hex, e.g. 0x80000000):\n"
      << "  -b=<addr>             add breakpoint (repeatable)\n"
      << "  -reset_vector=<addr>  initial PC\n"
      << "  -tohost=<addr>        tohost address (test ROM exit)\n"
      << "  -ram_start=<addr>     -ram_size=<size>\n"
      << "  -rom_start=<addr>     -rom_size=<size>\n"
      << "  -hd_start=<addr>      -hd_size=<size>\n"
      << "\n"
      << "Example:\n"
      << "  " << argv0 << " -firmware=fw.bin -reset_vector=0x80000000 -b=0x80000010 -d\n";
}



int main(int argc, char **argv) {

  // disabled forced args for testing
  if (argc < 2) {
    showUsage(argv[0]);
    return 1;
  } 

  static struct termios orig_term;


  // build configuration based on CLI arguments
  Config config = parse_arguments(argc, argv);
  std::cout << "Configuration: \n" << config.str() << std::endl;
  
  // setup
  Machine machine(config);
  machine.start();
  
  std::cout << "\nSTARTING EXECUTION...\n" << std::endl;

  while(!machine.halted()){
    machine.step();
  }
    
  // after the cpu has halted
  

  

  return 0;
}
