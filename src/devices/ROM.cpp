#include "ROM.hpp"

void ROM::load_firmware(){
  std::cout << "Loading firmware '" << get_config().firmware_path << "'...";

  if(get_config().firmware_path.empty()){
    Error<std::runtime_error>("Provide a firmware binary for startup");
  }

  std::ifstream f(get_config().firmware_path, std::ios::binary | std::ios::ate);
  
  if (!f)
    Error<std::runtime_error>("Could not open firmware file: '" +
                              get_config().firmware_path + "'");

  data.resize(static_cast<size_t>(f.tellg()));

  f.seekg(0);
  f.read(reinterpret_cast<char *>(data.data()), data.size());

  std::cout << "Done!" << std::endl;
}

u32 ROM::load(u32 address, BITSIZE size) {
  if (address >= data.size()) {
    // rest is simply = 0 so as to not run into an Error
    // TODO: raise an error?
    return 0x0;
  }

  u32 out = 0;
  for (int i = 0; i < size; i++) {
    out |= data.at(address + i) << (8 * i); // little-endian
  }

  return out;
}
