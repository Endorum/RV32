#ifndef CPU_HPP
#define CPU_HPP

// the CPU is NOT a Device like everything else
#include <cstdint>
class CPU {

public:
  CPU() {

    for (int i = 0; i < 32; i++)
      regfile[i] = 0;

    // or whereever the firmware starts.
    pc = 0x0;
  }

private:
  uint32_t regfile[32];
  void setReg(uint8_t idx, uint32_t val);
  uint32_t getReg(uint8_t idx);

  uint32_t pc;
};

#endif // CPU_HPP
