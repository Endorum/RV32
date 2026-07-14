#include <iostream>
#include <string>
#include <vector>

void showUsage(std::string argv0) {
  std::cout << "Usage:\n" << std::endl;
  std::cout << argv0 << "   [-mdsb] [options] " << std::endl;
  std::cout << "-m=[amf...]       : extensions | specify one or more modues, "
               "currenty available: none"
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
}

int main(int argc, char **argv) {

  if (argc < 2) {
    showUsage(argv[0]);
  }

  std::vector<std::string> args;

  return 0;
}
