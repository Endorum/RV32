#include "MACHINE.hpp"

void Machine::start() {

  std::cout << "Start..." << std::endl;

  rom.load_firmware();
  if(!config.harddisk_path.empty()) 
    hd.setup_harddisk();

  cpu.reset();
}

void Machine::step() {
  cpu.step();

  if(config.debug) debug();
  if(config.log) log();
}

void Machine::log() {
  std::cout << cpu.get_current_dis() << std::endl;
}

void Machine::debug() {
  std::cout << std::format("########## DEBUG CYCLE {} ##########\n", cpu.get_cycle()) << std::endl;
  std::cout << cpu.get_current_dis() << ": \n" << std::endl;
  std::cout << "CPU STATE: " << std::endl;
  std::cout << cpu.state_str() << std::endl;
  std::cout << "DEVICES: " << std::endl;
  
  

}
