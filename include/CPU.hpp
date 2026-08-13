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


class CPU{
public:
  CPU() = default;

  void reset();
  void step();

  void attach_bus(BUS* b);

  void set_config(const Config& c){ config = c; }

  std::string get_current_dis() { return current_dis; }
  u32 get_cycle(){return cycle;}
  std::string state_str();

  bool get_halted() { return halted; }

private:
  u32 regfile[32];
  u32 csr[4096];
  u32 pc;
  u64 cycle;

  BUS* bus = nullptr;

  Config config;
  std::string current_dis;
  u32 last_addr_used = 0x0;
  bool halted = false;

  u32 rs1_value;
  u32 rs2_value;

  void set_reg(u8 idx, u32 val);
  u32  get_reg(u8 idx) const;

  // mask for allowing and blocking csr writes
  u32 mask(u16 idx);
  void set_csr(u16 idx, u32 val);
  void set_csr(CSR_ADDR idx, u32 val) { set_csr(static_cast<u16>(idx), val); }
  
  u32  get_csr(u16 idx) const;
  u32  get_csr(CSR_ADDR idx) const { return get_csr( static_cast<u16>(idx)); }

  void store(u32 addr, BITSIZE size, u32 val);
  u32  load(u32 addr, BITSIZE size);

  void invalid_op(const Instruction& instr);
  void halt_if_deadlock(const Instruction& instr);
  bool valid_target(u32 target, const Instruction& instr);
  void jump(u32 target, const Instruction& instr);
  void execute(const Instruction& instr);

  u32 alu_add(u32 a, u32 b);
  u32 alu_sub(u32 a, u32 b);
  u32 alu_sll(u32 a, u32 b);
  u32 alu_slt(u32 a, u32 b);
  u32 alu_sltu(u32 a, u32 b);
  u32 alu_xor(u32 a, u32 b);
  u32 alu_srl(u32 a, u32 b);
  u32 alu_sra(u32 a, u32 b);
  u32 alu_or(u32 a, u32 b);
  u32 alu_and(u32 a, u32 b);

  void enter_trap(u32 trap_addr, TRAP_CODE code, u32 tval);
  void mret();
};


#endif // CPU_HPP
