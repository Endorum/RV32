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

  u32 fcsr;
  u64 fregfile[32];

  // needed for atomic instrs. (A Ext.)
  u32 reservation_addr = 0;
  bool reservation_valid = false;

  u32 rs1_value;
  u32 rs2_value;

  u64 fp_rs1; 
  u64 fp_rs2;

  void set_reg(u8 idx, u32 val);
  u32  get_reg(u8 idx) const;

  void set_freg(u8 idx, u64 val);
  u64  get_freg(u8 idx);
  u64  get_freg_s(u8 idx);

  void set_fcsr(u32 val);
  u32 get_fcsr();

  void set_rounding_mode(u8 mode);
  u8 get_rounding_mode();


  // mask for allowing and blocking csr writes
  u32 mask(u16 idx);
  void set_csr(u16 idx, u32 val);
  void set_csr(CSR_ADDR idx, u32 val) { set_csr(static_cast<u16>(idx), val); }
  
  u32  get_csr(u16 idx) const;
  u32  get_csr(CSR_ADDR idx) const { return get_csr( static_cast<u16>(idx)); }

  void store(u32 addr, BITSIZE size, u64 val);
  u64  load(u32 addr, BITSIZE size);

  u64 nanbox(u32 val);

  void invalid_op(const Instruction& instr);
  void halt_if_deadlock(const Instruction& instr);
  bool valid_target(u32 target, const Instruction& instr);
  void execute(const Instruction& instr);

  u32 alu_sll(u32 a, u32 b);
  u32 alu_slt(u32 a, u32 b);
  u32 alu_srl(u32 a, u32 b);
  u32 alu_sra(u32 a, u32 b);

  u32 alu_div(i32 a, i32 b);
  u32 alu_divu(u32 a, u32 b);
  u32 alu_rem(i32 a, i32 b);
  u32 alu_remu(u32 a, u32 b);

  // RV32F
  u32 alu_classify_s(float v);
  i32 fcvt_w_s(float v);
  u32 fcvt_wu_s(float v);

  float alu_fmin_s(float a, float b);
  float alu_fmax_s(float a, float b);

  // RV32D
  u64 alu_classify_d(double v);
  i64 fcvt_w_d(double v);
  u64 fcvt_wu_d(double v);

  double alu_fmin_d(double a, double b);
  double alu_fmax_d(double a, double b);


  void enter_trap(u32 trap_addr, TRAP_CODE code, u32 tval);
  void mret();
};


#endif // CPU_HPP
