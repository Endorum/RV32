#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

template <typename ExceptionType> [[noreturn]] void Error(std::string msg);
bool S_to_B(std::string str);

#endif // UTILS_HPP
