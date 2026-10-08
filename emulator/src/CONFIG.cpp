#include "../include/CONFIG.hpp"
#include "../include/UTILS.hpp"

Config parse_arguments(int argc, char** argv){
  Config config;
  for(int i=1;i<argc;i++){
    std::string arg = std::string(argv[i] + 1);
    if(arg == "d"){
      config.debug = true;
    }else if(arg == "l"){
      config.log = true;
    }else if(arg == "s"){
      config.step = true;
    }else{
      
      std::string left_side;
      std::string right_side;

      size_t pos = arg.find('=');
      if(pos != std::string::npos){
        left_side = arg.substr(0, pos);
        right_side = arg.substr(pos + 1);
      }else{
        Error<std::invalid_argument>("Expected right side after '='");
      }

      if(left_side == "b"){
        u32 bp = 0;
        try {
          bp = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid breakpoint address");
        }
        config.breakpoints.push_back(bp);
      }
      else if(left_side == "reset_vector"){
        try {
          config.reset_vector = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid reset_vector address");
        }
      }
      else if(left_side == "tohost"){
        try {
          config.tohost_address = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid tohost address");
        }
      }
      else if(left_side == "ram_start"){
        try {
          config.ram_start = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid ram_start address");
        }
      }
      else if(left_side == "ram_size"){
        try {
          config.ram_size = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid ram_size address");
        }
      }
      else if(left_side == "rom_start"){
        try {
          config.rom_start = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid rom_start address");
        }
      }
      else if(left_side == "rom_size"){
        try {
          config.rom_size = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid rom_size address");
        }
      }
      else if(left_side == "hd_start"){
        try {
          config.hd_start = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid hd_start address");
        }
      }
      else if(left_side == "hd_size"){
        try {
          config.hd_size = std::stoul(right_side.c_str(), nullptr, 16);
        }
        catch(std::exception E) {
          Error<std::invalid_argument>("Invalid hd_size address");
        }
      }
      else if(left_side == "firmware"){
        config.firmware_path = right_side;
      }else if(left_side == "harddisk"){
        config.harddisk_path = right_side;
      }else if(left_side == "removeable"){
        config.removable_path = right_side;
      }else{
        Error<std::invalid_argument>("Commandline option'" + left_side + "' not supported! ");
      }
    }
  }
  return config;
}

std::string breakpoints_str(const std::vector<u32> &bp) {
  std::string out = " [";

  for (auto v : bp) {
    out += std::format("0x{:x}, ", v);
  }

  // remove last ,
  if (!bp.empty())
    out = out.substr(0, out.length() - 2);

  return out + "]";
}
