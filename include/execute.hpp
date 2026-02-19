#pragma once

#include "defs.hpp"
#include "decode.hpp"

u32 alu_r_type(u32 src1, u32 src2, Instruction* instr);
u32 alu_i_type(u32 src1, Instruction* instr);