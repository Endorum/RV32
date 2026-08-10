#include "../include/CONFIG.hpp"
#include "../include/UTILS.hpp"

void parseArgument(CLIArgument arg, Config& config) {
  
  if (arg.S_type == "d") {
    config.debug = S_to_B(arg.S_value);
  }
  else if (arg.S_type == "l") {
    config.log = S_to_B(arg.S_value);
  }
  else if (arg.S_type == "s") {
    config.step = S_to_B(arg.S_value);
  } else if (arg.S_type == "b") {

    u32 bp = 0;
    try {
      bp = std::strtoul(arg.S_value.c_str(), nullptr, 16);
    } catch (std::exception E) {
      Error<std::invalid_argument>("Invalid breakpoint address");
    }

    config.breakpoints.push_back(bp);

  }

  else if (arg.S_type == "firmware") {
    config.firmware_path = arg.S_value;
  } else if (arg.S_type == "disk") {
    config.harddisk_path = arg.S_value;
  } else if (arg.S_type == "removable") {
    config.removable_path = arg.S_value;
  } else {
    Error<std::invalid_argument>("commandline option'" + arg.S_type +
                                 "' not supported! ");
  }
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
