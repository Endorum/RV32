
#include <cstring>

#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include "CPU.hpp"




void CPU::reset(){
  // clear registers
  for(int i=0;i<32;i++){
    regfile[i] = 0x000000000;
  }

  // clear csr
  for(int i=0;i<0x1000;i++){
    csr[i] = 0x000000000;
  }

  cycle = 0;


  // start at reset vector (start of ROM / firmware as of right now)
  pc = config.reset_vector;

  // Loading the extension and arch infos
  // (1 << 30) = (32 Bit) ? 1 : 0;
  // (1 << 8) = (I Ext.) ? 1 : 0;
  csr[(u16)CSR_ADDR::misa] = 
    MISA_XLEN_32 | 
    MISA_EXT_I | 
    MISA_EXT_M |
    MISA_EXT_A;

  // MPP liest ab reset immer 3 (= M-Mode)
  // ACHTUNG: für U Mode muss das wieder geändert werden!
  csr[(u16)CSR_ADDR::mstatus] = MSTATUS_MPP;

  reservation_valid = false;
  
}

void CPU::step(){

  // fetch
  u32 word = load(pc, WORD);

  

  // decode
  Instruction instr = decode(word, pc);

  if(config.debug || config.log){
    current_dis = disassemble(instr);
  }

  // preload registers
  rs1_value = get_reg(instr.rs1);
  rs2_value = get_reg(instr.rs2);

  // advance one word by default
  pc += 4;

  execute(instr);
  
  halt_if_deadlock(instr);
  cycle++;
}

void CPU::attach_bus(BUS* b){
  bus = b;
}

std::string CPU::state_str() {
  std::string out;
  out += std::format("\tPC: {:08X}\n", pc);

  out += "REGISTER: \n";
  out += std::format("\tx0:     {:08X}, x1/ra:  {:08X}, x2/sp:  {:08X}, x3/gp:  {:08X}\n",
  get_reg(0), get_reg(1), get_reg(2), get_reg(3));

  out += std::format("\tx4/tp:  {:08X}, x5/t0:  {:08X}, x6/t1:  {:08X}, x7/t2:  {:08X}\n",
      get_reg(4), get_reg(5), get_reg(6), get_reg(7));

  out += std::format("\tx8/s0:  {:08X}, x9/s1:  {:08X}, x10/a0: {:08X}, x11/a1: {:08X}\n",
      get_reg(8), get_reg(9), get_reg(10), get_reg(11));

  out += std::format("\tx12/a2: {:08X}, x13/a3: {:08X}, x14/a4: {:08X}, x15/a5: {:08X}\n",
      get_reg(12), get_reg(13), get_reg(14), get_reg(15));

  out += std::format("\tx16/a6: {:08X}, x17/a7: {:08X}, x18/s2: {:08X}, x19/s3: {:08X}\n",
      get_reg(16), get_reg(17), get_reg(18), get_reg(19));

  out += std::format("\tx20/s4: {:08X}, x21/s5: {:08X}, x22/s6: {:08X}, x23/s7: {:08X}\n",
      get_reg(20), get_reg(21), get_reg(22), get_reg(23));

  out += std::format("\tx24/s8: {:08X}, x25/s9: {:08X},    s10: {:08X},    s11: {:08X}\n",
      get_reg(24), get_reg(25), get_reg(26), get_reg(27));

  out += std::format("\tx28/t3: {:08X}, x29/t4: {:08X}, x30/t5: {:08X}, x31/t6: {:08X}\n",
      get_reg(28), get_reg(29), get_reg(30), get_reg(31));

  out += "PRELOADED REGISTERS:\n";
  out += std::format("\trs1: {:08X}\n", rs1_value);
  out += std::format("\trs2: {:08X}\n", rs2_value);

  out += std::format("LAST ADDRESS: \n\t{:08X}\n", last_addr_used);

  out += std::format("CSR: \n");
  out += std::format("\tmstatus: {:08X}, mtvec: {:08X}, mepc: {:08X}, mcause: {:08X}, mtval: {:08X}, mscratch: {:08X}", 
      get_csr(CSR_ADDR::mstatus), get_csr(CSR_ADDR::mtvec), get_csr(CSR_ADDR::mepc), get_csr(CSR_ADDR::mcause), get_csr(CSR_ADDR::mtval), get_csr(CSR_ADDR::mscratch));

  return out;
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

u32 CPU::mask(u16 idx) {
  CSR_ADDR addr = static_cast<CSR_ADDR>(idx);

  switch(addr){
    default: return 0xFFFFFFFF;
    
    // read only csr's
    case CSR_ADDR::misa:    return 0x00000000;
    case CSR_ADDR::tselect: return 0x00000000;

    case CSR_ADDR::mstatus: return MSTATUS_MIE | MSTATUS_MPIE; // 0x88


  }
}

void CPU::set_csr(u16 idx, u32 val) {
  const u32 m = mask(idx);
  csr[idx] = (val & m) | (csr[idx] & ~m);
}

u32 CPU::get_csr(u16 idx) const {
  return csr[idx];
}

void CPU::store(u32 addr, BITSIZE size, u32 val){
  if(!bus) Error<std::runtime_error>("BUS not assigned");
  last_addr_used = addr;
  bus->store(addr, size, val);
}

u32 CPU::load(u32 addr, BITSIZE size) {
  if(!bus) Error<std::runtime_error>("BUS not assigned");
  last_addr_used = addr;
  return bus->load(addr, size);
}


void CPU::invalid_op(const Instruction& instr){
  // handle invalid op via a trap instead of a C++ error
  enter_trap(instr.addr, TRAP_CODE::INVALID_OP, instr.word);
}

void CPU::halt_if_deadlock(const Instruction& instr) {
  
  // if word == 6F == j +0 == inf. loop AND interrupts are ->disabled<- then halt
  if(instr.word == 0x6F && !(get_csr(CSR_ADDR::mstatus) & MSTATUS_MIE)) {
    halted = true;
  }

}

bool CPU::valid_target(u32 target, const Instruction& instr) {
  
  // check for alignment
  if(target & 0x2){
    enter_trap(instr.addr, TRAP_CODE::MISALIGNED, target);
    return false;
  }

  return true;
}


void CPU::execute(const Instruction& instr){

  if(instr.op == Op::INVALID){
    invalid_op(instr);
    return;
  }
  
  if(instr.type == BaseType::ALU_R || instr.type == BaseType::ALU_I){
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


  else if(instr.type == BaseType::LOAD){
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


  else if(instr.type == BaseType::STORE){
    u32 addr = rs1_value + instr.imm;

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen) STORE");

      case Op::SB: store(addr, BYTE, rs2_value); break;
      case Op::SH: store(addr, HALF, rs2_value); break;
      case Op::SW: store(addr, WORD, rs2_value); break;

    } 
  }


  else if(instr.type == BaseType::BRANCH){
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

      if(valid_target(target, instr)){
        pc = target;
      }

    }

  }

  else if(instr.op == Op::JAL){

    u32 target = instr.addr + instr.imm;

    if(valid_target(target, instr)){
      pc = target;
      set_reg(instr.rd, instr.addr + 4);
    }

  }
  
  else if(instr.op == Op::JALR){
    u32 target = ((i32)rs1_value + instr.imm) & ~1;

    if(valid_target(target, instr)){
      pc = target;
      set_reg(instr.rd, instr.addr + 4);
    }

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
    

    if(instr.op == Op::ECALL){
      enter_trap(instr.addr, TRAP_CODE::ECALL, 0);
    }
    
    else if(instr.op == Op::EBREAK){
      enter_trap(instr.addr, TRAP_CODE::EBREAK, instr.addr);
    }

    else if(instr.op == Op::WFI){
      // wfi = wait for interrup = do nothing for now
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

  else if(instr.type == BaseType::ATOMIC){
    
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


  else{
    Error<std::runtime_error>("Unhandled execute?");
  }

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
  if(b == 0x0){
    return -1;
  }

  return a / b;
}

u32 CPU::alu_divu(u32 a, u32 b) {
  if(b == 0x0){
    return 0xFFFFFFFF;
  }
  return a / b;
}

u32 CPU::alu_rem(i32 a, i32 b) {
  if(b == 0x0){
    return a;
  }

  return a % b;
}

u32 CPU::alu_remu(u32 a, u32 b) {
  if(b == 0x0){
    return a;
  }

  return a % b;
}

void CPU::enter_trap(u32 trap_addr, TRAP_CODE cause, u32 tval) {
  set_csr(CSR_ADDR::mepc, trap_addr);       // mepc <- addr of the ecall
  set_csr(CSR_ADDR::mcause, (u32)cause);    // 3: ebreak, 11: ecall Machine mode, 8: ecall User mode
  set_csr(CSR_ADDR::mtval, tval);

  u32 mstatus = get_csr(CSR_ADDR::mstatus);

  u32 mie = (mstatus >> 3) & 1;                         // aktuelles MIE
  mstatus = (mstatus & ~(MSTATUS_MPIE)) | (mie << 7);   // MPIE <- MIE
  mstatus &= ~(MSTATUS_MIE);                            // MIE <- 0 (handler läuft ungestört)
  mstatus |= (MSTATUS_MPP);                             // MPP <- 3 (aus m-mode)

  set_csr(CSR_ADDR::mstatus, mstatus);

  u32 mtvec = get_csr(CSR_ADDR::mtvec);
  pc = mtvec & ~0x3;

  // why?
  reservation_valid = false;
}

void CPU::mret() {
  u32 mstatus = get_csr(CSR_ADDR::mstatus);
  
  u32 mpie = (mstatus >> 7) & 1;                      // MIE herauslesen
  mstatus = (mstatus & ~(MSTATUS_MIE)) | (mpie << 3); // MIE <- MPIE wiederherstellen
  mstatus |= (MSTATUS_MPIE);                          // MPIE <- 1

  set_csr(CSR_ADDR::mstatus, mstatus);

  pc = get_csr(CSR_ADDR::mepc) & ~0x1;
  
}
