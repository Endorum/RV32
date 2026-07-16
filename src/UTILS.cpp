#include <iostream>

#include "../include/UTILS.hpp"

template <typename ExceptionType> [[noreturn]] void Error(std::string msg) {

  ExceptionType exc(msg);

  std::cerr << "\tERROR: " << exc.what() << "\n\t";

  throw exc;
}

bool S_to_B(std::string str) {

  std::transform(str.begin(), str.end(), str.begin(), ::toupper);

  if (str == "TRUE")
    return true;
  if (str == "FALSE")
    return false;

  Error<std::invalid_argument>("Could not convert '" + str + "' to boolean");
}
