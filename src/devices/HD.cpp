#include <iostream>
#include <thread>

#include "UTILS.hpp"
#include "HD.hpp"


void HD::setup_harddisk() {
  filepath = get_config().harddisk_path;
  
  std::cout << "Loading Harddisk '" << filepath << "'...";


  file.open(filepath, std::ios::binary | std::ios::in | std::ios::out);

  if(!file.is_open()){
    std::ofstream create(filepath, std::ios::binary); // legt leere Datei an
    if(!create) Error<std::runtime_error>("Could not create disk file: '" + filepath + "'");
    create.close();
    file.open(filepath, std::ios::binary | std::ios::in | std::ios::out);
  }

  if (!file)
    Error<std::runtime_error>("Could not open harddisk file: '" +
                              filepath + "'");

  std::cout << "Done!" << std::endl;
}



u32 HD::load(u32 addr, BITSIZE size) {
  delay();  

  u8 buf[4] = {};
  file.seekg(addr);

  file.read(reinterpret_cast<char*>(buf), size); // liest evtl weniger als size
  file.clear(); // eof/fail zurücksetzen  stream bleibt benutzbar

  u32 value = 0;
  for(u32 i=0;i<size;i++){
    value |= static_cast<u32>(buf[i]) << (8 * i);
  }

  return value;
}

void HD::store(u32 addr, BITSIZE size, u32 value) {
  delay();

  u8 buf[4];
  
  for(u32 i=0;i<size;i++){
    buf[i] = (value >> (8 * i)) & 0xFF; // little endian zerlegen
  } 

  file.seekp(addr); // p = put zeiger (nicht seek->g<-)
  file.write(reinterpret_cast<char*>(buf), size);
  file.flush();
}


void HD::delay() {
  std::this_thread::sleep_for(std::chrono::microseconds(delay_mus));
}