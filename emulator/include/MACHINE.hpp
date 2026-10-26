#ifndef MACHINE_HPP
#define MACHINE_HPP

#include <memory>
#include <string>
#include <termios.h>
#include <unistd.h>

#include "BUS.hpp"
#include "CONFIG.hpp"
#include "CPU.hpp"
#include "DEFS.hpp"
#include "memory_map.h"

#include "devices/RAM.hpp"
#include "devices/ROM.hpp"
#include "devices/HD.hpp"
#include "devices/TOHOST.hpp"
#include "devices/CLINT.hpp"
#include "devices/UART.hpp"




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

        // attach clint to cpu
        cpu.attach_clint(&clint);

        // bus attachements order = priority eg tohost is inside of ram, needs to be
        // bevor ram so that the bus routes correctly
        // add devices
        bus.addDevice(rom);
        bus.addDevice(tohost);
        bus.addDevice(ram);
        bus.addDevice(clint);
        bus.addDevice(uart);
    

        // only add harddrive if a path is given otherwise the space is simply not reachable
        if(!config.harddisk_path.empty()) bus.addDevice(hd);

        init_terminal();
    
    

    }

    void start();
    void step();

    bool halted();

    // minimal debug, show single disass line
    CPU_STATE get_cpu_state() {return cpu.get_state(); }
    std::string curr_instr() { return cpu.get_current_instr(); }
    void log();
    void debug();
  
    void reset();
  
    std::string uart_output(){
            return uart.take_tx();
    }
  
    void uart_input(const std::string& s) {
            for (char c : s){
                    uart.push_rx(u8(c));
            }
    }
 
private:

    void init_terminal();

    Config config;
  
    BUS bus;
    CPU cpu;

    ROM rom{config.rom_start, config.rom_size};
    RAM ram{config.ram_start, config.ram_size};
    HD  hd{config.hd_start, config.hd_size};
    TOHOST tohost{config.tohost_address, 0x100};
    CLINT clint{config.clint_start, config.clint_size};

  
    UART uart{config.uart_start, config.uart_size};
  
};

#endif // MACHINE_HPP
