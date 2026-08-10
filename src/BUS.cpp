#include "BUS.hpp"

void BUS::addDevice(Device& dev) {

  std::cout << "Adding Device: " << dev.str()  << std::endl;

  devices.push_back(&dev);
}

u32 BUS::load(u32 address, BITSIZE size) const {

  for(auto& dev : devices){
    u32 start = dev->get_start();
    u32 mem_size = dev->get_size();

    if(start <= address && start + mem_size >= (address + size)){
      return dev->load(address - start, size);
    }

  }

  Error<std::out_of_range>(
      std::format("Address: {:08X} - {:08X} not mapped", address, address + size));

  return -1;
}

void BUS::store(u32 address, BITSIZE size, u32 value) {
    
  for(auto& dev : devices){
    u32 start = dev->get_start();
    u32 mem_size = dev->get_size();

    if(start <= address && start + mem_size >= (address + size)){
      dev->store(address - start, size, value);
      return;
    }

  }

  Error<std::out_of_range>(
      std::format("Address: {:08X} - {:08X} not mapped", address, address + size));

}

std::string BUS::str() const {
  std::string out =
      "BUS with " + std::to_string(devices.size()) + " attached devices:\n";
  for (auto &dev : devices) {
    out += dev->str() + "\n";
  }
  return out;
}
