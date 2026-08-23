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
#include "CLINT.hpp"


// interrupt bit not set


enum class ROUNDING_MODE : u8 {
  INV, // invalid
  RNE, // Round to nearest, ties to even
  RTZ, // Round towards Zero
  RDN, // Round Down (towards -\infty)
  RUP, // Round Up (towards +\infty)
  RMM, // Round to Nearest, ties to Max Magnitude
  DYN, // In Instr.'s ->rm<- Field, selects dynamic rounding mode
};

enum PREC{
  PREC_SINGLE = 0b00,
  PREC_DOUBLE = 0b01,
  PREC_HALF   = 0b10,
  PREC_QUAD   = 0b11
};

enum class PRIV_LEVEL : u8 {
  USER       = 0b00, // U
  SUPERVISOR = 0b01, // S
  RESERVED   = 0b10, 
  MACHINE    = 0b11  // M
};

class CPU{
public:
  CPU() = default;

  void reset();
  void step();

  void attach_bus(BUS* b);
  void attach_clint(CLINT* c);

  void set_config(const Config& c){ config = c; }

  std::string get_current_dis() { return current_dis; }
  u32 get_cycle(){ return get_csr(CSR_ADDR::mcycle); }
  std::string state_str();

  bool get_halted() { return halted; }
  bool get_sleeping() { return sleeping; }

private:
  u32 regfile[32];
  u32 csr[4096];
  u32 pc;

  BUS* bus = nullptr;
  CLINT* clint = nullptr;

  Config config;

  std::string current_dis;
  
  u32 last_addr_used = 0x0;
  
  bool halted = false;
  bool sleeping = false;

  u32 fcsr;
  u64 fregfile[32];

  PRIV_LEVEL priv;

  // needed for atomic instrs. (A Ext.)
  u32 reservation_addr = 0;
  bool reservation_valid = false;

  Instruction instr;

  u32 rs1_value;
  u32 rs2_value;

  // rv32f/d
  u64 fp_rs1; 
  u64 fp_rs2;

  void set_reg(u8 idx, u32 val);
  u32  get_reg(u8 idx) const;

  void set_freg(u8 idx, u64 val);
  u64  get_freg(u8 idx);
  u32  get_freg_s(u8 idx);

  // mask for allowing and blocking csr writes
  u32 mask(u16 idx);
  void set_csr(u16 idx, u32 val);
  void set_csr(CSR_ADDR idx, u32 val) { set_csr(static_cast<u16>(idx), val); }
  void set_csr_raw(CSR_ADDR idx, u32 val) { csr[(u16)idx] = val; }
  
  u32  get_csr(u16 idx) const;
  u32  get_csr(CSR_ADDR idx) const { return get_csr( static_cast<u16>(idx)); }

  void store(u32 addr, u8 size, u64 val);
  u64  load(u32 addr, u8 size);

  u64 nanbox(u32 val);

  void halt_if_deadlock();
  bool valid_target(u32 target);
  void execute();

  u32 alu_sll(u32 a, u32 b);
  u32 alu_slt(u32 a, u32 b);
  u32 alu_srl(u32 a, u32 b);
  u32 alu_sra(u32 a, u32 b);

  u32 alu_div(i32 a, i32 b);
  u32 alu_divu(u32 a, u32 b);
  u32 alu_rem(i32 a, i32 b);
  u32 alu_remu(u32 a, u32 b);

  // RV32F
  ROUNDING_MODE get_rm();
  
  void fp_begin(ROUNDING_MODE mode);
  void fp_end();

  u32 alu_classify_s(float v);
  i32 fcvt_w_s(float v);
  u32 fcvt_wu_s(float v);

  float alu_fmin_s(float a, float b);
  float alu_fmax_s(float a, float b);

  float canon_nan_s(float in);

  bool is_snan_s(float f);

  // RV32D
  u64 alu_classify_d(double v);
  i64 fcvt_w_d(double v);
  u64 fcvt_wu_d(double v);

  double alu_fmin_d(double a, double b);
  double alu_fmax_d(double a, double b);

  double canon_nan_d(double in);

  bool is_snan_d(double d);

  void enter_exception(u32 addr, EXCEPTION_CODE code, u32 word);
  void enter_interrupt(u32 addr, INTERRUPT_CODE code);
  void enter_trap(u32 trap_addr, u32 code, u32 tval);
  void mret();  


  /* RV32I */
  void exec_alu();
  void exec_load();
  void exec_store();
  void exec_branch();
  void exec_jal();
  void exec_jalr();
  void exec_lui();
  void exec_auipc();
  void exec_fence();
  void exec_system();

  /* RV32A */
  void exec_atomic();

  /* RV32 FP */
  void exec_load_fp();
  void exec_store_fp();
  void exec_fp_fused();
  void exec_fp_alu_single();
  void exec_fp_alu_double();
};




#endif // CPU_HPP
