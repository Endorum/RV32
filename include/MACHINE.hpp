#ifndef MACHINE_HPP
#define MACHINE_HPP

#include "BUS.hpp"
#include "CONFIG.hpp"
#include "CPU.hpp"
#include "DEFS.hpp"
#include "MAP.h"
#include "RAM.hpp"
#include "ROM.hpp"
#include <memory>

class Machine {
public:
  Machine(const Config& c) : config(c) {

    cpu.set_config(config);
    rom.set_config(config);
    ram.set_config(config);

    // attach bus to cpu
    cpu.attach_bus(&bus);

    // add devices
    bus.addDevice(rom);
    bus.addDevice(ram);
  }

  void start();
  void step();
  void debug();

private:

  Config config;
  
  BUS bus;
  CPU cpu;

  ROM rom{ROM_START, ROM_SIZE};
  RAM ram{RAM_START, RAM_SIZE};
  
};

#endif // MACHINE_HPP
