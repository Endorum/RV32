#ifndef MACHINE_HPP
#define MACHINE_HPP

#include <memory>

#include "BUS.hpp"
#include "CONFIG.hpp"
#include "CPU.hpp"
#include "DEFS.hpp"
#include "MAP.h"

#include "RAM.hpp"
#include "ROM.hpp"
#include "HD.hpp"
#include "TOHOST.hpp"



class Machine {
public:
  Machine(const Config& c) : config(c) {

    cpu.set_config(config);
    rom.set_config(config);
    ram.set_config(config);
    hd.set_config(config);
    tohost.set_config(config);
    

    // attach bus to cpu
    cpu.attach_bus(&bus);

    // bus attachements order = priority eg tohost is inside of ram, needs to be
    // bevor ram so that the bus routes correctly
    // add devices
    bus.addDevice(rom);
    bus.addDevice(tohost);
    bus.addDevice(ram);

    // only add harddrive if a path is given otherwise the space is simply not reachable
    if(!config.harddisk_path.empty()) bus.addDevice(hd);

    
  }

  void start();
  void step();

  // minimal debug, show single disass line
  void log();
  void debug();

private:

  Config config;
  
  BUS bus;
  CPU cpu;

  ROM rom{config.rom_start, config.rom_size};
  RAM ram{config.ram_start, config.ram_size};
  HD  hd{config.hd_start, config.hd_size};
  TOHOST tohost{config.tohost_address, 0x100};
  
};

#endif // MACHINE_HPP
