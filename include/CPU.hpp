#ifndef CPU_HPP
#define CPU_HPP

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <format>

#include "BUS.hpp"
#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "UTILS.hpp"


struct IF_ID {
  Instruction instr_cache;
};

struct ID_EX {
  u32 src1_value;
  u32 src2_value;
};

struct EX_MEM {
  u32 alu_result;
  u32 link_value;
  bool branch;
};

struct MEM_WB {
  u32 mem_result;
};

// the CPU is NOT a Device like everything else
class CPU {

public:
  CPU(Bus &b) : bus(b) {}

  void reset() {
    for (int i = 0; i < 32; i++)
      regfile[i] = 0;

    pc = T_config.resetVector;
  }

  void step() {

    if (T_config.B_debug) {

      printf("#################################################################"
             "#####################################\n");
      printf("CPU CYCLE %llu: \n", cycle);
    }

    IF();
    ID();
    EX();
    MEM();
    WB();

    if (T_config.B_debug) {
      std::cout << "Dissassembled Instruction: " << std::endl;
      std::cout << "\n" + instr_cache.line() + "\n" << std::endl;
      std::cout << instr_cache.str() << std::endl;
      std::cout << str() << std::endl;
    } else {
      // show anyway
      std::cout << instr_cache.line() << std::endl;
    }

    cycle++;
  }

  std::string str() {
    return std::format("Register File:\n") +
           std::format("    x0:     {:08X}, x1/ra:  {:08X}, x2/sp:  {:08X}, "
                       "x3/gp:  {:08X}\n",
                       getReg(0), getReg(1), getReg(2), getReg(3)) +

           std::format("    x4/tp:  {:08X}, x5/t0:  {:08X}, x6/t1:  {:08X}, "
                       "x7/t2:  {:08X}\n",
                       getReg(4), getReg(5), getReg(6), getReg(7)) +

           std::format("    x8/s0:  {:08X}, x9/s1:  {:08X}, x10/a0: {:08X}, "
                       "x11/a1: {:08X}\n",
                       getReg(8), getReg(9), getReg(10), getReg(11)) +

           std::format("    x12/a2: {:08X}, x13/a3: {:08X}, x14/a4: {:08X}, "
                       "x15/a5: {:08X}\n",
                       getReg(12), getReg(13), getReg(14), getReg(15)) +

           std::format("    x16/a6: {:08X}, x17/a7: {:08X}, x18/s2: {:08X}, "
                       "x19/s3: {:08X}\n",
                       getReg(16), getReg(17), getReg(18), getReg(19)) +

           std::format("    x20/s4: {:08X}, x21/s5: {:08X}, x22/s6: {:08X}, "
                       "x23/s7: {:08X}\n",
                       getReg(20), getReg(21), getReg(22), getReg(23)) +

           std::format("    x24/s8: {:08X}, x25/s9: {:08X},    s10: {:08X},    "
                       "s11: {:08X}\n",
                       getReg(24), getReg(25), getReg(26), getReg(27)) +

           std::format("    x28/t3: {:08X}, x29/t4: {:08X}, x30/t5: {:08X}, "
                       "x31/t6: {:08X}\n",
                       getReg(28), getReg(29), getReg(30), getReg(31)) +

           std::format("PC: {:08X} \n", pc) +
           std::format("cycle: {:08d} \n", cycle);
  }

private:
  u32 regfile[32];
  u32 csr[4096];

  u32 pc;
  uint64_t cycle = 0;

  void IF();

  Instruction instr_cache;

  void ID();

  u32 src1_value;
  u32 src2_value;

  void EX();

  u32 alu_result;
  u32 link_value;
  bool branch;
  bool jump;

  void MEM();

  u32 mem_result;

  void WB();
  Bus &bus;

  void setReg(u8 idx, u32 val) {
    if (idx == 0)
      return;
    if (idx >= 32) {
      Error<std::invalid_argument>("Invalid index for register write");
    }
    regfile[idx] = val;
  }

  u32 getReg(u8 idx) {
    if (idx == 0)
      return 0;
    if (idx >= 32) {
      Error<std::invalid_argument>("Invalid index for register read");
    }
    return regfile[idx];
  }

  void setCSR(u16 idx, u32 val) {
    // may check if the idx is valid, buts not important rn
    csr[idx] = val;
  }

  u32 getCSR(u16 idx) { return csr[idx]; }

  u32 read(u32 address, BITSIZE size) { return bus.read(address, size); }

  void write(u32 address, BITSIZE size, u32 value) {
    bus.write(address, size, value);
  }
};

u32 alu_r_type(u32 src1, u32 src2, Instruction *instr);
u32 alu_i_type(u32 src1, Instruction *instr);
bool evaluate_branch(u32 src1, u32 src2, Instruction *instr);
u32 compute_u_type(Instruction *instr);

#endif // CPU_HPP
