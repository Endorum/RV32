#include "MACHINE.hpp"



void Machine::start() {

  std::cout << "Start..." << std::endl;

  rom.load_firmware();

  if(!config.harddisk_path.empty()) 
    hd.setup_harddisk();


  // temp!! copy rom content to ram for spike test
  for(int i=0;i<config.rom_size;i++){
    u8 rom_b = rom.load(i, BYTE);
    ram.store(i, BYTE, rom_b);
  }

  cpu.reset();
}


void Machine::step() {
  cpu.step();

  clint.tick();
  uart.tick();
  
  if(config.debug) debug();
  if(config.log) log();



  
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

// disable canonical terminal mode, usally it echoes every byte and waits for enter
// for the uart we want it to read the input immediatly and not echo it back
void Machine::init_terminal() {
  if(!isatty(STDIN_FILENO)) return;

  tcgetattr(STDIN_FILENO, &orig_term); // save og
  struct termios raw = orig_term; 
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  atexit(restore_terminal); // after an exit, restore terminal is called
}
