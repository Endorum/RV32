

#include <cstring>
#include <cfenv>

#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include "CPU.hpp"

/// RV32I
void CPU::exec_alu(){
  u32 result = 0;
  u32 A = rs1_value;
  u32 B = (instr.type == BaseType::ALU_I) ? (u32)instr.imm : rs2_value;

  switch(instr.op){
    default: Error<std::runtime_error>(std::format("Invalid op type (should not happen) ALU {}", (u32)instr.op));

    case Op::ADD: 
    case Op::ADDI:  result = A + B; break;

    case Op::SUB:   result = A - B; break;
    
    case Op::XOR: 
    case Op::XORI:  result = A ^ B; break;
    
    case Op::OR: 
    case Op::ORI:   result = A | B; break;
    
    case Op::AND: 
    case Op::ANDI:  result = A & B; break;
    
    case Op::SLL: 
    case Op::SLLI:  result = alu_sll(A, B); break;
    
    case Op::SRL: 
    case Op::SRLI:  result = alu_srl(A, B); break;
    
    case Op::SRA: 
    case Op::SRAI:  result = alu_sra(A, B); break;
    
    case Op::SLT: 
    case Op::SLTI:  result = alu_slt(A, B); break;
    
    case Op::SLTU: 
    case Op::SLTIU: result = (A < B) ? 1 : 0; break;

    case Op::MUL:     result = A * B; break;
    case Op::MULH:    result = (u32)(((i64)(i32)A * (i64)(i32)B) >> 32); break;
    case Op::MULHSU:  result = (u32)(((i64)(i32)A * (u64)B)      >> 32); break;
    case Op::MULHU:   result = (u32)(((u64)A      * (u64)B)      >> 32); break;

    case Op::DIV:     result = alu_div(A, B); break;
    case Op::DIVU:    result = alu_divu(A, B); break;

    case Op::REM:     result = alu_rem(A, B); break;
    case Op::REMU:    result = alu_remu(A, B); break;

  }

  set_reg(instr.rd, result);
}

void CPU::exec_load(){
  u32 value = 0;
  u32 addr  = rs1_value + (u32)instr.imm;

  switch(instr.op){
    default: Error<std::runtime_error>("Invalid op type (should not happen) LOAD");

    case Op::LB: value = sign_extend(load(addr, BYTE), 8); break;
    case Op::LH: value = sign_extend(load(addr, HALF), 16); break;
    case Op::LW: value = load(addr, WORD); break;
    case Op::LBU: value = load(addr, BYTE); break;
    case Op::LHU: value = load(addr, HALF); break;

  }

  set_reg(instr.rd, value);
}

void CPU::exec_store(){
  u32 addr = rs1_value + instr.imm;

  switch(instr.op){
    default: Error<std::runtime_error>("Invalid op type (should not happen) STORE");

    case Op::SB: store(addr, BYTE, rs2_value); break;
    case Op::SH: store(addr, HALF, rs2_value); break;
    case Op::SW: store(addr, WORD, rs2_value); break;

  } 
}

void CPU::exec_branch(){
  bool branch = false;

  i32 sA = static_cast<i32>(rs1_value);
  i32 sB = static_cast<i32>(rs2_value);

  switch(instr.op){
    default: Error<std::runtime_error>("Invalid op type (should not happen)");

    case Op::BEQ:   branch = (sA == sB); break;
    case Op::BNE:   branch = (sA != sB); break;
    case Op::BLT:   branch = (sA <  sB); break;
    case Op::BGE:   branch = (sA >= sB); break;
    case Op::BLTU:  branch = (rs1_value <  rs2_value); break;
    case Op::BGEU:  branch = (rs1_value >= rs2_value); break;

  }

  if(branch){
    
    u32 target = instr.addr + instr.imm;

    if(valid_target(target)){
      pc = target;
    }

  }
}

void CPU::exec_jal(){
  u32 target = instr.addr + instr.imm;

  if(valid_target(target)){
    pc = target;
    set_reg(instr.rd, instr.addr + 4);
  }
}

void CPU::exec_jalr(){
  u32 target = ((i32)rs1_value + instr.imm) & ~1;

  if(valid_target(target)){
    pc = target;
    set_reg(instr.rd, instr.addr + 4);
  }
}

void CPU::exec_lui(){
  set_reg(instr.rd, (u32)instr.imm << 12);
}

void CPU::exec_auipc(){
  set_reg(instr.rd, instr.addr + (instr.imm << 12) );
}

void CPU::exec_fence(){
  // NOP for now
}

void CPU::exec_system(){
  if(instr.op == Op::ECALL){
    switch(priv){
      default:
        enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
        break;
      case PRIV_LEVEL::USER:
        enter_exception(instr.addr, EXCEPTION_CODE::ECALL_FROM_U_MODE, 0);
        break;
      case PRIV_LEVEL::SUPERVISOR:
        enter_exception(instr.addr, EXCEPTION_CODE::ECALL_FROM_S_MODE, 0);
        break;
      case PRIV_LEVEL::MACHINE:
        enter_exception(instr.addr, EXCEPTION_CODE::ECALL_FROM_M_MODE, 0);
        break;
    }
    
  }
  
  else if(instr.op == Op::EBREAK){
    enter_exception(instr.addr, EXCEPTION_CODE::BREAKPOINT, instr.addr);
  }

  else if(instr.op == Op::WFI){
    sleeping = true;
    return;
  }

  else if(instr.op == Op::MRET){
    mret();
  }

  else if(instr.op >= Op::CSRRW && instr.op <= Op::CSRRCI) {
    
    u16 csr_addr = instr.imm & 0xFFF;
    u32 old_csr = get_csr(csr_addr);
    u8 uimm = instr.rs1;

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");

      case Op::CSRRW:   
        set_csr(csr_addr, rs1_value);
        break;
        
      case Op::CSRRS:   
        if(instr.rs1 == 0) break;
        set_csr(csr_addr, old_csr | rs1_value);
        break;

      case Op::CSRRC:   
        if(instr.rs1 == 0) break;
        set_csr(csr_addr, old_csr & ~rs1_value);
        break;

      case Op::CSRRWI:   
        set_csr(csr_addr, uimm);
        break;
        
      case Op::CSRRSI:   
        if(instr.rs1 == 0) break;
        set_csr(csr_addr, old_csr | uimm);
        break;

      case Op::CSRRCI:   
        if(instr.rs1 == 0) break;
        set_csr(csr_addr, old_csr & ~uimm);
        break;

    }
    set_reg(instr.rd, old_csr);
  }
  else{
    Error<std::runtime_error>("Invalid SYSTEM op type (should not happen)");
  }
}

/// RV32A
void CPU::exec_atomic(){

  if(instr.op == Op::LR){
    u32 word = load(rs1_value, WORD); 
    set_reg(instr.rd, word);

    reservation_addr = rs1_value;
    reservation_valid = true;
    return;
  }
  
  else if(instr.op == Op::SC){
    
    if(reservation_valid && reservation_addr == rs1_value) {
      store(rs1_value, WORD, rs2_value);
      set_reg(instr.rd, 0);
      reservation_valid = false;
    }else{
      set_reg(instr.rd, 1);
      reservation_valid = false;
    }
    
    return;

  }else{
    u32 value = load(rs1_value, WORD);
    u32 ret_val = 0;

    if(instr.op >= Op::AMOSWAP && instr.op <= Op::AMOMAXU) set_reg(instr.rd, value);

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");
      
      case Op::AMOSWAP:
        ret_val = rs2_value;
        break;
      
      case Op::AMOADD:
        ret_val = value + rs2_value;
        break;
      
      case Op::AMOAND:
        ret_val = value & rs2_value;
        break;

      case Op::AMOOR:
        ret_val = value | rs2_value;
        break;

      case Op::AMOXOR:
        ret_val = value ^ rs2_value;
        break;

      case Op::AMOMIN:
        ret_val = ((i32)value < (i32)rs2_value) ? value : rs2_value; 
        break;
      
      case Op::AMOMINU:
        ret_val = (value < rs2_value) ? value : rs2_value;
        break;

      case Op::AMOMAX:
        ret_val = ((i32)value > (i32)rs2_value) ? value : rs2_value;
        break;
      
      case Op::AMOMAXU:
        ret_val = (value > rs2_value) ? value : rs2_value;
        break;
        
        
    }

    store(rs1_value, WORD, ret_val);

  }
}


// utils and helber for RV32I/M/A ext.
bool CPU::valid_target(u32 target) {
  
  // check for alignment
  if(target & 0x2){
    enter_exception(instr.addr, EXCEPTION_CODE::INSTR_ADDR_MISALIGNED, target);
    return false;
  }

  return true;
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

u32 CPU::alu_srl(u32 a, u32 b){
  u32 shift_amnt = b & 0x1F;
  return a >> shift_amnt;
}

u32 CPU::alu_sra(u32 a, u32 b){
  u32 shift_amnt = b & 0x1F;
  i32 s_a = static_cast<i32>(a);
  return s_a >> shift_amnt;
}

u32 CPU::alu_div(i32 a, i32 b) {
  if(b == 0){ // div by 0
    return -1;
  }

  if(a == INT32_MIN && b == -1){ 
    return INT32_MIN;
  }

  return a / b;
}

u32 CPU::alu_divu(u32 a, u32 b) {
  if(b == 0){
    return UINT32_MAX;
  }

  return a / b;
}

u32 CPU::alu_rem(i32 a, i32 b) {
  if(b == 0){
    return a;
  }

  if(a == INT32_MIN && b == -1){
    return 0;
  }

  return a % b;
}

u32 CPU::alu_remu(u32 a, u32 b) {
  if(b == 0){
    return a;
  }

  return a % b;
}
