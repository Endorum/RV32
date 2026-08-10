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

private:
  u32 regfile[32];
  u32 csr[4096];
  u32 pc;
  u64 cycle;

  BUS* bus = nullptr;

  Config config;

  u32 rs1;
  u32 rs2;

  void set_reg(u8 idx, u32 val);
  u32  get_reg(u8 idx) const;

  void set_csr(u16 idx, u32 val);
  u32  get_csr(u16 idx) const;

  void store(u32 addr, BITSIZE size, u32 val);
  u32  load(u32 addr, BITSIZE size);

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

};


#endif // CPU_HPP
