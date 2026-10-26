#include "BUS.hpp"
#include <ERROR.hpp>

void BUS::addDevice(Device& dev) {

  std::cout << "Adding Device: " << dev.str()  << std::endl;

  devices.push_back(&dev);
}

// returns the device accessed by the address, based on the memory configuration
Device* BUS::route(u32 address, u8 size) const {
  
  for(Device* dev : devices){
    u32 start = dev->get_start();
    u32 dev_size = dev->get_size();

    if(address >= start){
      u32 offset = address - start; // relativ addr

      if(offset < dev_size){
        // address is within device

        if(size <= dev_size - offset){
          // rest of device is big enough for the requested size
          return dev;
        }

      }

    }

  }

  return nullptr;

} 

u32 BUS::load(u32 address, u8 size) const {
  
  Device* dev = route(address, size);

  if(!dev){
    throw TrapException(EXCEPTION_CODE::LOAD_ACCESS_FAULT, address);
  }


  u32 rel_addr = address - dev->get_start();
  return dev->load(rel_addr, size);

}

void BUS::store(u32 address, u8 size, u32 value) {
    
  Device* dev = route(address, size);

  if(!dev){
    throw TrapException(EXCEPTION_CODE::STORE_ACCESS_FAULT, address);
  }

  u32 rel_addr = address - dev->get_start();
  dev->store(rel_addr, size, value);

}

std::string BUS::str() const {
  std::string out =
      "BUS with " + std::to_string(devices.size()) + " attached devices:\n";
  for (auto &dev : devices) {
    out += dev->str() + "\n";
  }
  return out;
}
