#include "MACHINE.hpp"

void Machine::start() {

  rom.load_firmware();

  cpu.reset();
}

void Machine::step()
{
  cpu.step();
}

void Machine::debug(){
  
}