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
  Machine() : cpu(bus) {

    // add devices
    bus.addDevice("ROM", ROM_START, ROM_SIZE, std::make_unique<ROM>());
    bus.addDevice("RAM", RAM_START, RAM_SIZE, std::make_unique<RAM>());

    cpu.reset();
  }

  void step() { cpu.step(); }

  void debug() { std::cout << str() << std::endl; }
  std::string str() {
    return "CPU:\n" + cpu.str() + "\nBUS:\n" + bus.str() + "\nDEVICES:\n" +
           bus.devices_string();
  }

private:
  Bus bus;
  CPU cpu;
};

#endif // MACHINE_HPP
