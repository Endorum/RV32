#pragma once

#include <stdexcept>
#include "CPU.hpp"

class TrapException : public std::exception{

public:
  explicit TrapException(EXCEPTION_CODE c) : code(c), tval(0), tval_addr(false) {}
  explicit TrapException(EXCEPTION_CODE c, u32 t) : code(c), tval(t), tval_addr(true) {}
  EXCEPTION_CODE get_code() const { return code; }

  bool get_tval_addr() const { return tval_addr; }
  u32 get_tval() const { return tval; }

private:
  EXCEPTION_CODE code;
  u32 tval;

  // tval_addr is true if the Exception came from loading or 
  // storing memory, in which case tval is != 0 but instead the address
  // that was faulty.
  bool tval_addr = false;
};