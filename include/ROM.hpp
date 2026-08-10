#ifndef ROM_HPP
#define ROM_HPP

#include "CONFIG.hpp"
#include "DEVICE.hpp"
#include "UTILS.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

class ROM : public Device {

public:
  explicit ROM(u32 start, u32 size) : Device(start, size, "ROM") {}

  void load_firmware(){
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

  u32 load(u32 address, BITSIZE size) override;

  void store(u32 address, BITSIZE size, u32 value) override {
    // Error<std::invalid_argument>("ROM is read only!");
    // raise error?
  }

private:
  std::vector<u8> data;
};

#endif // ROM_HPP
