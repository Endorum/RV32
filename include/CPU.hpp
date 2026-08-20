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
enum class EXCEPTION_CODE : u32{
  INSTR_ADDR_MISALIGNED = 0,
  INSTR_ACCESS_FAULT    = 1,
  ILLEGAL_INSTRUCTION   = 2,
  BREAKPOINT            = 3,
  LOAD_ADDR_MISALIGNED  = 4,
  LOAD_ACCESS_FAULT     = 5,
  STORE_ADDR_MISALIGNED = 6,
  STORE_ACCESS_FAULT    = 7,
  ECALL_FROM_U_MODE     = 8,
  ECALL_FROM_S_MODE     = 9,
  ECALL_FROM_M_MODE     = 11,
  INSTR_PAGE_FAULT      = 12,
  LOAD_PAGE_FAULT       = 13,
  STORE_PAGE_FAULT      = 15,
  DOUBLE_TRAP           = 16,
  SOFTWARE_CHECK        = 18,
  HARDWARE_ERROR        = 19,
};

// interrupt bit set
enum class INTERRUPT_CODE : u32 {
  SUP_SOFTWARE_INT    = 0x80000001,
  MACH_SOFTWARE_INT   = 0x80000003,
  SUP_TIMER_INT       = 0x80000005,
  MACH_TIMER_INT      = 0x80000007,
  SUP_EXT_INT         = 0x80000009,
  MACH_EXT_INT        = 0x8000000B,
  COUNT_OVERFLOW_INT  = 0x8000000D,
};

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

  // needed for atomic instrs. (A Ext.)
  u32 reservation_addr = 0;
  bool reservation_valid = false;

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

  void set_fcsr(u32 val);
  u32 get_fcsr();

  ROUNDING_MODE get_rm(const Instruction& instr);
  
  void fp_begin(ROUNDING_MODE mode);
  void fp_end();

  


  // mask for allowing and blocking csr writes
  u32 mask(u16 idx);
  void set_csr(u16 idx, u32 val);
  void set_csr(CSR_ADDR idx, u32 val) { set_csr(static_cast<u16>(idx), val); }
  
  u32  get_csr(u16 idx) const;
  u32  get_csr(CSR_ADDR idx) const { return get_csr( static_cast<u16>(idx)); }

  void store(u32 addr, u8 size, u64 val);
  u64  load(u32 addr, u8 size);

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
};




#endif // CPU_HPP
