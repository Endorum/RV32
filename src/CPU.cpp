
#include <cstring>

#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include "CPU.hpp"




void CPU::reset(){
  // clear registers
  for(int i=0;i<32;i++){
    regfile[i] = 0x0;
  }

  // start at reset vector (start of ROM / firmware as of right now)
  pc = config.reset_vector;
  cycle = 0;
}

void CPU::step(){

  // fetch
  u32 word = load(pc, WORD);

  // decode
  Instruction instr = decode(word, pc);

  // preload registers
  rs1 = get_reg(instr.rs1);
  rs2 = get_reg(instr.rs2);

  // advance one word by default
  pc += 4;

  execute(instr);
  
  cycle++;
}

void CPU::attach_bus(BUS* b){
  bus = b;
}

// internal 

void CPU::set_reg(u8 idx, u32 val){
  if(idx == 0) return;
  if(idx > 31) return; // TODO: ERROR instead?

  regfile[idx] = val;
}

u32 CPU::get_reg(u8 idx) const {
  if(idx == 0) return 0;
  if(idx > 31) return 0; // TODO: ERROR instead?

  return regfile[idx];
}

void CPU::set_csr(u16 idx, u32 val) {
  csr[idx] = val;
}

u32 CPU::get_csr(u16 idx) const {
  return csr[idx];
}

void CPU::store(u32 addr, BITSIZE size, u32 val){
  if(!bus) Error<std::runtime_error>("BUS not assigned");
  bus->store(addr, size, val);
}

u32 CPU::load(u32 addr, BITSIZE size) {
  if(!bus) Error<std::runtime_error>("BUS not assigned");
  return bus->load(addr, size);
}

void CPU::execute(const Instruction& instr){

  if(instr.op == Op::INVALID) Error<std::runtime_error>("Op was of type INVALID");
  
  if(instr.type == BaseType::ALU_R || instr.type == BaseType::ALU_I){
    u32 result = 0;
    u32 A = rs1;
    u32 B = (instr.type == BaseType::ALU_I) ? (u32)instr.imm : rs2;

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");

      case Op::ADD: 
      case Op::ADDI:  result = alu_add(A, B); break;

      case Op::SUB:   result = alu_sub(A, B); break;
      
      case Op::XOR: 
      case Op::XORI:  result = alu_xor(A, B); break;
      
      case Op::OR: 
      case Op::ORI:   result = alu_or(A, B); break;
      
      case Op::AND: 
      case Op::ANDI:  result = alu_and(A, B); break;
      
      case Op::SLL: 
      case Op::SLLI:  result = alu_sll(A, B); break;
      
      case Op::SRL: 
      case Op::SRLI:  result = alu_srl(A, B); break;
      
      case Op::SRA: 
      case Op::SRAI:  result = alu_sra(A, B); break;
      
      case Op::SLT: 
      case Op::SLTI:  result = alu_slt(A, B); break;
      
      case Op::SLTU: 
      case Op::SLTIU: result = alu_sltu(A, B); break;

    }

    set_reg(instr.rd, result);
  }
  else if(instr.type == BaseType::LOAD){
    u32 value = 0;
    u32 addr  = rs1 + (u32)instr.imm;

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");

      case Op::LB: value = sign_extend(load(addr, BYTE), 8); break;
      case Op::LH: value = sign_extend(load(addr, HALF), 16); break;
      case Op::LW: value = load(addr, WORD); break;
      case Op::LBU: value = load(addr, BYTE); break;
      case Op::LHU: value = load(addr, HALF); break;

    }

    set_reg(instr.rd, value);
  }
  else if(instr.type == BaseType::STORE){
    u32 addr = rs1 + instr.imm;

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");

      case Op::SB: store(addr, BYTE, rs2); break;
      case Op::SH: store(addr, HALF, rs2); break;
      case Op::SW: store(addr, WORD, rs2); break;

    } 
  }
  else if(instr.type == BaseType::BRANCH){
    bool branch = false;

    i32 sA = static_cast<i32>(rs1);
    i32 sB = static_cast<i32>(rs2);

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");

      case Op::BEQ:   branch = (sA == sB); break;
      case Op::BNE:   branch = (sA != sB); break;
      case Op::BLT:   branch = (sA <  sB); break;
      case Op::BGE:   branch = (sA >= sB); break;
      case Op::BLTU:  branch = (rs1 <  rs2); break;
      case Op::BGEU:  branch = (rs1 >= rs2); break;

    }

    if(branch)
      pc = instr.addr + instr.imm;

  }
  else if(instr.op == Op::JAL){
    set_reg(instr.rd, instr.addr + 4);

    pc = instr.addr + instr.imm;
  }
  else if(instr.op == Op::JALR){
    set_reg(instr.rd, instr.addr + 4);

    pc = (((i32)rs1 + instr.imm) & ~1);
  }
  else if(instr.op == Op::LUI){
    set_reg(instr.rd, (u32)instr.imm << 12);
  }
  else if(instr.op == Op::AUIPC){
    set_reg(instr.rd, instr.addr + (instr.imm << 12) );
  }
  else if(instr.op == Op::FENCE){
    // NOP for now
  }
  else if(instr.type == BaseType::SYSTEM){
    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");
      case Op::ECALL: 
      case Op::EBREAK: 
      case Op::CSRRW: 
      case Op::CSRRS: 
      case Op::CSRRC: 
        Error<std::runtime_error>("Nothing here yet");
    }
  }
  else{
    Error<std::runtime_error>("Unhandled execute?");
  }

}

u32 CPU::alu_add(u32 a, u32 b){
  return a + b;
}

u32 CPU::alu_sub(u32 a, u32 b){
  return a - b;
}

u32 CPU::alu_sll(u32 a, u32 b){
  u32 shift_amnt = b & 0x1F;
  return a << shift_amnt;
}

u32 CPU::alu_slt(u32 a, u32 b){
  i32 s_a = static_cast<i32>(a);
  i32 s_b = static_cast<i32>(b);
  return (s_a < s_b) ? 1 : 0;
}

u32 CPU::alu_sltu(u32 a, u32 b){
  return (a < b) ? 1 : 0;
}

u32 CPU::alu_xor(u32 a, u32 b){
  return a ^ b;
}

u32 CPU::alu_srl(u32 a, u32 b){
  u32 shift_amnt = b & 0x1F;
  return a >> shift_amnt;
}

u32 CPU::alu_sra(u32 a, u32 b){
  u32 shift_amnt = b & 0x1F;
  i32 s_a = static_cast<i32>(a);
  return s_a >> shift_amnt;
}

u32 CPU::alu_or(u32 a, u32 b){
  return a | b;
}

u32 CPU::alu_and(u32 a, u32 b){
  return a & b;
}
