#pragma once

#include "stdint.h"


#define READ_REG(n) asm volatile("mv %0, " #n : "=r"(value))
#define WRITE_REG(n, v) asm volatile("mv " #n ", %0" :: "r"(v) : #n)

#define csr_read(reg)({ unsigned long __v; asm volatile("csrr %0, " #reg : "=r"(__v)); __v; })
#define csr_write(reg, val) asm volatile("csrw " #reg ", %0" :: "rK"(val))


// register index
typedef enum
{
  zero,
  ra,
  sp,
  gp,
  tp,
  t0,
  t1,
  t2,
  s0,
  s1,
  a0,
  a1,
  a2,
  a3,
  a4,
  a5,
  a6,
  a7,
  s2,
  s3,
  s4,
  s5,
  s6,
  s7,
  s8,
  s9,
  s10,
  s11,
  t3,
  t4,
  t5,
  t6
} REG_IDX;

uint32_t get_reg(uint8_t idx);
void set_reg(uint8_t idx, uint32_t value);
uint32_t read_pc(void);
uint32_t read_sp(void);
void write_sp(uint32_t value);
void write_pc(uint32_t value);

void jump_to(uint32_t pc, uint32_t sp);

void dump();



uint8_t read_byte(uint32_t addr);
uint32_t read_word(uint32_t addr);

void    write_byte(uint32_t addr, uint8_t value);
void write_word(uint32_t addr, uint32_t word);