#include "MACHINE.hpp"
#include <csignal>



void Machine::start() {

    std::cout << "Start..." << std::endl;

    reset();
}


void Machine::step() {
    cpu.step();

    clint.tick();
    uart.tick();
  
    if(config.debug) debug();
    if(config.log && !cpu.get_sleeping()) log();



  
}

bool Machine::halted() {
  
    if(cpu.get_halted()){  
        std::cout << "\n\nHALTED after " << cpu.get_cycle() << " cycles\n";
        std::cout << "CPU STATE:" << std::endl;
        std::cout << cpu.state_str() << std::endl; 
        return true;
    }

  

    return false;
}

void Machine::log() {
    std::cout << cpu.get_current_dis() << std::endl;
}

void Machine::debug() {
    std::cout << std::format("########## DEBUG CYCLE {} ##########\n", cpu.get_cycle()) << std::endl;
    std::cout << cpu.get_current_dis() << ": \n" << std::endl;
    std::cout << "CPU STATE: " << std::endl;
    std::cout << cpu.state_str() << std::endl;


}

static struct termios orig_term;

static void restore_terminal(){
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

// Ctrl-C / kill skip atexit, so restore the terminal here too (tcsetattr is async-signal-safe)
static void restore_terminal_and_exit(int sig){
    restore_terminal();
    _exit(128 + sig);
}

// disable canonical terminal mode, usally it echoes every byte and waits for enter
// for the uart we want it to read the input immediatly and not echo it back
void Machine::init_terminal() {
    if(!isatty(STDIN_FILENO)) return;

    tcgetattr(STDIN_FILENO, &orig_term); // save og
    struct termios raw = orig_term; 
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    atexit(restore_terminal); // after an exit, restore terminal is called
    signal(SIGINT, restore_terminal_and_exit);
    signal(SIGTERM, restore_terminal_and_exit);
}

void Machine::reset() {
    
        std::cout << "\n\nMACHINE RESET\n\n" << std::endl;
    
        rom.load_firmware();

        if(!config.harddisk_path.empty() && !hd.was_setup())
            hd.setup_harddisk();

        cpu.reset();
    
}
