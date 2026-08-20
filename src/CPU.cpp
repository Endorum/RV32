
#include <cstring>
#include <cfenv>

#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include "CPU.hpp"




void CPU::reset(){
  // clear registers
  for(int i=0;i<32;i++){
    regfile[i] = 0x000000000;
  }

  // clear fp32 registers
  for(int i=0;i<32;i++){
    fregfile[i] = 0x000000000;
  }

  // clear csr
  for(int i=0;i<0x1000;i++){
    csr[i] = 0x000000000;
  }

  // clear fcsr
  fcsr = 0x0;

  // done in csr = 0x00000000 already
  // cycle = 0;


  // start at reset vector (start of ROM / firmware as of right now)
  pc = config.reset_vector;

  // Loading the extension and arch infos
  csr[(u16)CSR_ADDR::misa] = 
    MISA_MXL_RV32 |
    MISA_EXT_I | 
    MISA_EXT_M |
    MISA_EXT_A |
    MISA_EXT_F |
    MISA_EXT_D;

  // MPP liest ab reset immer 3 (= M-Mode)
  // ACHTUNG: für U Mode muss das wieder geändert werden!
  // set FS = 11 = dirty
  csr[(u16)CSR_ADDR::mstatus] = MSTATUS_MPP_MASK | MSTATUS_FS_DIRTY;

  reservation_valid = false;
  
}

void CPU::step(){

  // wake up if interrupt
  if (sleeping){
    if((clint->pending_mip() & get_csr(CSR_ADDR::mie)) != 0){
      sleeping = false;
      // continue with normal execution
    }
    else {
      // the CPU is "sleeping", it skips the decoding and waits until mtime >= mtimecmp
      // to wakeup
      // vvv count cycles while sleeping too!
      set_csr(CSR_ADDR::mcycle, get_csr(CSR_ADDR::mcycle) + 1 );
      return; 
    } 
  }

  // check for clint interrupts
  u32 mip = clint->pending_mip();
  u32 pending = mip & get_csr(CSR_ADDR::mie);
  if((get_csr(CSR_ADDR::mstatus) & MSTATUS_MIE) && pending){

    // software (Bit 3) has priority before timer (Bit 7)
    if(pending & MIP_MSIP)      enter_interrupt(pc, INTERRUPT_CODE::MACH_SOFTWARE_INT);
    else if(pending & MIP_MTIP) enter_interrupt(pc, INTERRUPT_CODE::MACH_TIMER_INT);

    return;
  }

  // fetch...
  u32 word = load(pc, WORD);

  // ...decode...
  Instruction instr = decode(word, pc);

  if(config.debug || config.log){
    current_dis = disassemble(instr);
  }

  // preload registers
  rs1_value = get_reg(instr.rs1);
  rs2_value = get_reg(instr.rs2);

  // preload fp registers
  fp_rs1 = get_freg(instr.rs1);
  fp_rs2 = get_freg(instr.rs2);

  // if FS = 00 = off
  if((get_csr(CSR_ADDR::mstatus) & MSTATUS_FS_MASK) == MSTATUS_FS_OFF){
    
    // ... and trying to execute an FP instr. -> trap
    if(
      instr.type == BaseType::LOAD_FP ||
      instr.type == BaseType::STORE_FP ||
      instr.type == BaseType::FMADD ||
      instr.type == BaseType::FMSUB ||
      instr.type == BaseType::FNMSUB ||
      instr.type == BaseType::FNMADD ||
      instr.type == BaseType::FP_ALU
      ){
      enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
      return;
    }

    // ... and trying to change the FP status register -> trap
    if(instr.op >= Op::CSRRW && instr.op <= Op::CSRRCI){
      if(instr.imm == (u16)CSR_ADDR::fcsr || instr.imm == (u16)CSR_ADDR::fflags || instr.imm == (u16)CSR_ADDR::frm){
        enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
        return;
      }
    }
  }

  // advance one word by default
  pc += 4;

  // ...execute
  execute(instr);
  
  halt_if_deadlock(instr);
  
  // count cycles in the mcsr register
  set_csr(CSR_ADDR::mcycle, get_csr(CSR_ADDR::mcycle) + 1 );
}

void CPU::attach_bus(BUS* b){
  bus = b;
}

void CPU::attach_clint(CLINT *c) {
  clint = c;
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
  if(idx > 31) return;

  regfile[idx] = val;
}

u32 CPU::get_reg(u8 idx) const {
  if(idx == 0) return 0;
  if(idx > 31) return 0;

  return regfile[idx];
}

void CPU::set_freg(u8 idx, u64 val) {
  csr[(u16)CSR_ADDR::mstatus] |= MSTATUS_FS_DIRTY; // set FS = 11 = dirty

  fregfile[idx] = val;
}

u64 CPU::get_freg(u8 idx) {
  return fregfile[idx];
}

u32 CPU::get_freg_s(u8 idx) {
  u64 f = get_freg(idx);
  return (f >> 32) == 0xFFFFFFFF ? (u32)f : 0x7FC00000;
}

void CPU::set_fcsr(u32 val) {
  fcsr = val;
}

u32 CPU::get_fcsr() {
  return fcsr;
}

ROUNDING_MODE CPU::get_rm(const Instruction& instr) {

  switch(instr.rm){
    default:  
      enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
      return ROUNDING_MODE::INV;
    case 0b000: return ROUNDING_MODE::RNE;
    case 0b001: return ROUNDING_MODE::RTZ;
    case 0b010: return ROUNDING_MODE::RDN;
    case 0b011: return ROUNDING_MODE::RUP;
    case 0b100: return ROUNDING_MODE::RMM;

    // dynamic, read frm from fcsr
    case 0b111: {
      u8 frm = get_csr(CSR_ADDR::frm);
      
      if(frm >= 5) {
        enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
        return ROUNDING_MODE::INV;
      }
        
      
      // copy instr but change the stored rm
      Instruction tmp = instr;
      tmp.rm = frm;
      return get_rm(tmp);
    }
      
  }

  return ROUNDING_MODE::INV;
}

void CPU::fp_begin(ROUNDING_MODE mode) {
  int host_mode = 0;

  switch(mode){
    
    default: 
    case ROUNDING_MODE::INV:
      return;

    case ROUNDING_MODE::RNE: host_mode = FE_TONEAREST; break;
    case ROUNDING_MODE::RTZ: host_mode = FE_TOWARDZERO; break;
    case ROUNDING_MODE::RDN: host_mode = FE_DOWNWARD; break;
    case ROUNDING_MODE::RUP: host_mode = FE_UPWARD; break;
    case ROUNDING_MODE::RMM: host_mode = FE_TONEAREST; break; // tiny difference then the spec, only on exact ties

    case ROUNDING_MODE::DYN:
      Error<std::runtime_error>("Dynamic rounding mode, get_rm() needs to be called beforehand");
      break; // shouldnt happen
  }
  
  std::fesetround(host_mode);
  std::feclearexcept(FE_ALL_EXCEPT);
}

void CPU::fp_end() {
  fcsr |= std::fetestexcept(FE_INVALID)   ? FCSR_NV : 0x00; // NV
  fcsr |= std::fetestexcept(FE_DIVBYZERO) ? FCSR_DZ : 0x00; // DZ
  fcsr |= std::fetestexcept(FE_OVERFLOW)  ? FCSR_OF : 0x00; // OF
  fcsr |= std::fetestexcept(FE_UNDERFLOW) ? FCSR_UF : 0x00; // UF
  fcsr |= std::fetestexcept(FE_INEXACT)   ? FCSR_NX : 0x00; // NX

  // return to default host mode
  std::fesetround(FE_TONEAREST);
}

u32 CPU::mask(u16 idx) {
  CSR_ADDR addr = static_cast<CSR_ADDR>(idx);

  switch(addr){
    default: return 0xFFFFFFFF;
    
    // read only csr's
    case CSR_ADDR::misa:    return 0x00000000;
    case CSR_ADDR::tselect: return 0x00000000;

    case CSR_ADDR::mstatus: 
      return MSTATUS_MIE | MSTATUS_MPIE | MSTATUS_FS_MASK;


  }
}

void CPU::set_csr(u16 idx, u32 val) {
  const u32 m = mask(idx);

  if(idx == (u16)(CSR_ADDR::fflags)){
    csr[(u16)CSR_ADDR::mstatus] |= MSTATUS_FS_DIRTY; // set FS = 11 = dirty
    fcsr = (fcsr & ~0x1F) | (val & 0x1F);
  }else if(idx == (u16)(CSR_ADDR::frm)){
    csr[(u16)CSR_ADDR::mstatus] |= MSTATUS_FS_DIRTY; // set FS = 11 = dirty
    fcsr = (fcsr & ~0xE0) | ((val & 0x7) << 5);
  }else if(idx == (u16)(CSR_ADDR::fcsr)){
    csr[(u16)CSR_ADDR::mstatus] |= MSTATUS_FS_DIRTY; // set FS = 11 = dirty
    fcsr = (fcsr & ~0xFF) | (val & 0xFF);
  }else if(idx == (u16)(CSR_ADDR::mtvec)){

    if((val & 0x3) >= 2){ // invalid / unhandled interrup modes
      csr[idx] = (val & ~0x3); // write mode = 0 instead
    }

    csr[idx] = val;

  }
  
  else{
    csr[idx] = (val & m) | (csr[idx] & ~m);
  }
  
}

u32 CPU::get_csr(u16 idx) const {

  if(idx == (u16)(CSR_ADDR::fflags)){
    return fcsr & 0x1F;
  }else if(idx == (u16)(CSR_ADDR::frm)){
    return (fcsr & 0xE0) >> 5;
  }else if(idx == (u16)(CSR_ADDR::fcsr)){
    return fcsr & 0xFF;
  }else if(idx == (u16)(CSR_ADDR::mstatus)){
    u32 v = csr[idx];
    if(((v >> 13) & 0b11) == 0b11){ // <=> FS = 0b11 = dirty?
      v |= (1u << 31); // set SD = 1;
    }
    return v; 
  }else if(idx == (u16)(CSR_ADDR::mip)){
    return clint->pending_mip();
  }

  else{
    return csr[idx];
  }
}

void CPU::store(u32 addr, u8 size, u64 val){
  if(!bus) Error<std::runtime_error>("BUS not assigned");
  last_addr_used = addr;

  if(size == DOUBLE){
    bus->store(addr, WORD, (val & 0x00000000FFFFFFFF));
    bus->store(addr + 4,     WORD, (val >> 32));
  }else
    bus->store(addr, size, val);
}

u64 CPU::load(u32 addr, u8 size) {
  if(!bus) Error<std::runtime_error>("BUS not assigned");
  last_addr_used = addr;

  if(size == DOUBLE){
    u64 val = (((u64)bus->load(addr + 4, WORD)) << 32);
        val |= bus->load(addr, WORD);
    return val;
  }else
    return bus->load(addr, size);
  
}

u64 CPU::nanbox(u32 val) {
  return 0xFFFFFFFF00000000ULL | (u32)(val);
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
    enter_exception(instr.addr, EXCEPTION_CODE::INSTR_ADDR_MISALIGNED, target);
    return false;
  }

  return true;
}


void CPU::execute(const Instruction& instr){

  if(instr.op == Op::INVALID){
    enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
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
  
  else if(instr.type == BaseType::FENCE){
    // NOP for now
  }
  
  else if(instr.type == BaseType::SYSTEM){
    

    if(instr.op == Op::ECALL){
      enter_exception(instr.addr, EXCEPTION_CODE::ECALL_FROM_M_MODE, 0); //TODO: based on mode, currently only M mode
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

  /* RV32A */
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

  /* RV32F/D/H/Q */
  else if(instr.type == BaseType::LOAD_FP){
    u32 addr = rs1_value + (u32)instr.imm;
    switch(instr.op){
      case Op::FLW: {
        u64 val = load(addr, WORD) | 0xFFFFFFFF00000000;
        set_freg(instr.rd, val);
        break;
      }
      case Op::FLD: {
        u64 val = load(addr, DOUBLE);
        set_freg(instr.rd, val);
        break;
      }
      default: Error<std::runtime_error>("Not handled width for load fp");
    }
  }

  else if(instr.type == BaseType::STORE_FP){
    u32 addr = rs1_value + instr.imm;
    switch(instr.op){
      case Op::FSW:
        store(addr, WORD, fp_rs2);
        break;
      
      case Op::FSD:
        store(addr, DOUBLE, fp_rs2);
        break;

      default: Error<std::runtime_error>("Not handled width for store fp");

    }
  }

  /* RV32F */
  else if(instr.type == BaseType::FMADD || instr.type == BaseType::FMSUB || instr.type == BaseType::FNMSUB || instr.type == BaseType::FNMADD){
    
    ROUNDING_MODE rm = get_rm(instr);
    if(rm == ROUNDING_MODE::INV) return;
    fp_begin(rm);
    
    if(instr.width == PREC_SINGLE){
      float f_rs1 = std::bit_cast<float>((u32)(get_freg_s(instr.rs1)));
      float f_rs2 = std::bit_cast<float>((u32)(get_freg_s(instr.rs2)));
      float f_rs3 = std::bit_cast<float>((u32)(get_freg_s(instr.rs3)));  
      float result;
           if(instr.op == Op::FMADD_S) result = std::fma(f_rs1, f_rs2, f_rs3);
      else if(instr.op == Op::FMSUB_S) result = std::fma(f_rs1, f_rs2, -f_rs3);
      else if(instr.op == Op::FNMSUB_S) result = std::fma(-f_rs1, f_rs2, f_rs3);
      else if(instr.op == Op::FNMADD_S) result = std::fma(-f_rs1, f_rs2, -f_rs3);
      else Error<std::runtime_error>("Invalid fp operation");

      set_freg(instr.rd, nanbox(std::bit_cast<u32>(canon_nan_s(result))));

    }
    else if(instr.width == PREC_DOUBLE){
      double f_rs1 = std::bit_cast<double>((u64)(fp_rs1));
      double f_rs2 = std::bit_cast<double>((u64)(fp_rs2));
      double f_rs3 = std::bit_cast<double>((u64)(get_freg(instr.rs3)));

      double result;
           if(instr.op == Op::FMADD_D) result = std::fma(f_rs1, f_rs2, f_rs3);
      else if(instr.op == Op::FMSUB_D) result = std::fma(f_rs1, f_rs2, -f_rs3);
      else if(instr.op == Op::FNMSUB_D) result = std::fma(-f_rs1, f_rs2, f_rs3);
      else if(instr.op == Op::FNMADD_D) result = std::fma(-f_rs1, f_rs2, -f_rs3);
      else Error<std::runtime_error>("Invalid fp operation");


      set_freg(instr.rd, std::bit_cast<u64>(canon_nan_d(result)));

    }
    else{
      Error<std::runtime_error>("Not yet implemented");
    }
    
    fp_end();
    
  }

  else if(instr.type == BaseType::FP_ALU && instr.width == PREC_SINGLE){

    if(instr.op == Op::FSGNJ_S || instr.op == Op::FSGNJN_S || instr.op == Op::FSGNJX_S || instr.op == Op::FMV_W_X){

      u32 a = get_freg_s(instr.rs1);
      u32 b = get_freg_s(instr.rs2);

      // Bit pattern operations
      switch(instr.op){
        default: Error<std::runtime_error>("Invalid op type (should not happen)");

        // injecting b's sign into a in different way
        case Op::FSGNJ_S: 
          set_freg(instr.rd, nanbox((a & 0x7FFFFFFF) | (b & 0x80000000)));
          break;
        case Op::FSGNJN_S: 
          set_freg(instr.rd, nanbox((a & 0x7FFFFFFF) | ((~b) & 0x80000000)));
          break;
        case Op::FSGNJX_S: 
          set_freg(instr.rd, nanbox(a ^ (b & 0x80000000)));
          break;

        // just copy the actual bytes from the int. reg. to an freg.
        case Op::FMV_W_X: set_freg(instr.rd, nanbox(rs1_value)); break;
      }
    }

    else if(
      instr.op == Op::FEQ_S || instr.op == Op::FLT_S || instr.op == Op::FLE_S ||
      instr.op == Op::FCLASS_S || 
      instr.op == Op::FCVT_W_S || instr.op == Op::FCVT_WU_S || instr.op == Op::FMV_X_W
    ){
      // result goes into an int. register
      float f_rs1 = std::bit_cast<float>((u32)(get_freg_s(instr.rs1)));
      float f_rs2 = std::bit_cast<float>((u32)(get_freg_s(instr.rs2)));
      u32 result;

      ROUNDING_MODE rm = ROUNDING_MODE::INV;

      if(instr.op == Op::FCVT_W_S || instr.op == Op::FCVT_WU_S) {
        rm = get_rm(instr);
        if(rm == ROUNDING_MODE::INV) return;
        fp_begin(rm);
      }
        

      switch(instr.op){
        
        case Op::FEQ_S:     
          if(is_snan_s(f_rs1) || is_snan_s(f_rs2)) 
            fcsr |= FCSR_NV;
          result = (f_rs1 == f_rs2) ? 1 : 0; 
          break;

        case Op::FLT_S:     
          if(std::isnan(f_rs1) || std::isnan(f_rs2))
            fcsr |= FCSR_NV;
          result = (f_rs1 <  f_rs2) ? 1 : 0; 
          break;

        case Op::FLE_S:     
          if(std::isnan(f_rs1) || std::isnan(f_rs2))
            fcsr |= FCSR_NV;
          result = (f_rs1 <= f_rs2) ? 1 : 0; 
          break;

        case Op::FCLASS_S:  result = alu_classify_s(f_rs1); break;

        // fp -> int
        case Op::FCVT_W_S:  result = fcvt_w_s(f_rs1); break;
        case Op::FCVT_WU_S: result = fcvt_wu_s(f_rs1); break;

        // copy freg bytes to int. reg
        case Op::FMV_X_W:   result = (u32)(get_freg(instr.rs1)); break;
        default: Error<std::runtime_error>("Invalid op type (should not happen)");
      }

      if(instr.op == Op::FCVT_W_S || instr.op == Op::FCVT_WU_S) {
        fp_end();
      }

      set_reg(instr.rd, result);
    }

    else if(
      instr.op == Op::FADD_S    ||
      instr.op == Op::FSUB_S    ||
      instr.op == Op::FMUL_S    ||
      instr.op == Op::FDIV_S    ||
      instr.op == Op::FSQRT_S   ||
      instr.op == Op::FMIN_S    ||
      instr.op == Op::FMAX_S    ||
      instr.op == Op::FCVT_S_W  ||
      instr.op == Op::FCVT_S_WU ||
      instr.op == Op::FCVT_S_D
      
    ){

      // result goes into a fp register
      float f_rs1 = std::bit_cast<float>((u32)(get_freg_s(instr.rs1)));
      float f_rs2 = std::bit_cast<float>((u32)(get_freg_s(instr.rs2)));
      float result;

      ROUNDING_MODE rm = ROUNDING_MODE::INV;

      if(instr.op != Op::FMIN_S && instr.op != Op::FMAX_S) {
        rm = get_rm(instr);
        if(rm == ROUNDING_MODE::INV) return;
        fp_begin(rm);
      }
        
      
      switch(instr.op){
        default: Error<std::runtime_error>("Invalid op type (should not happen)");
        case Op::FADD_S:  result = f_rs1 + f_rs2; break;
        case Op::FSUB_S:  result = f_rs1 - f_rs2; break;
        case Op::FMUL_S:  result = f_rs1 * f_rs2; break;
        case Op::FDIV_S:  result = f_rs1 / f_rs2; break;
        case Op::FSQRT_S: result = sqrtf(f_rs1); break;
        case Op::FMIN_S:  
          result = alu_fmin_s(f_rs1, f_rs2); break;
        case Op::FMAX_S:  result = alu_fmax_s(f_rs1, f_rs2); break;

        // int -> fp
        case Op::FCVT_S_W:  result = (float)((i32)rs1_value); break;
        case Op::FCVT_S_WU: result = (float)(rs1_value); break;

        // d -> f
        case Op::FCVT_S_D: result = (float)(std::bit_cast<double>(fp_rs1)); break;
      
      }
    
      if(instr.op != Op::FMIN_S && instr.op != Op::FMAX_S){
        fp_end();
      }

      set_freg(instr.rd, nanbox(std::bit_cast<u32>(canon_nan_s(result))));

    }
    else{
      Error<std::runtime_error>("Invalid FP SINGLE instruction");
    }

    
    
  }

  /* RV32D */
  else if(instr.type == BaseType::FP_ALU && instr.width == PREC_DOUBLE){
    
    if(instr.op == Op::FSGNJ_D || instr.op == Op::FSGNJN_D || instr.op == Op::FSGNJX_D){

      u64 a = fp_rs1;
      u64 b = fp_rs2;

      // Bit pattern operations
      switch(instr.op){
        default: Error<std::runtime_error>("Invalid op type (should not happen)");

        // injecting b's sign into a in different way
        case Op::FSGNJ_D: 
          set_freg(instr.rd, (a & 0x7FFFFFFFFFFFFFFF) | (b & 0x8000000000000000));
          break;
        case Op::FSGNJN_D: 
          set_freg(instr.rd, (a & 0x7FFFFFFFFFFFFFFF) | ((~b) & 0x8000000000000000));
          break;
        case Op::FSGNJX_D: 
          set_freg(instr.rd, a ^ (b & 0x8000000000000000));
          break;

      }
    }

    else if(
      instr.op == Op::FEQ_D || instr.op == Op::FLT_D || instr.op == Op::FLE_D ||
      instr.op == Op::FCLASS_D || 
      instr.op == Op::FCVT_W_D || instr.op == Op::FCVT_WU_D){ 

      double f_rs1 = std::bit_cast<double>(fp_rs1);
      double f_rs2 = std::bit_cast<double>(fp_rs2);
      u64 result;

      ROUNDING_MODE rm = ROUNDING_MODE::INV;

      if(instr.op == Op::FCVT_W_D || instr.op == Op::FCVT_WU_D) {
        rm = get_rm(instr);
        if(rm == ROUNDING_MODE::INV) return;
        fp_begin(rm);
      }
        

      switch(instr.op){
        default: Error<std::runtime_error>("Invalid op type (should not happen)");
        
        case Op::FEQ_D:     
          if(is_snan_d(f_rs1) || is_snan_d(f_rs2)) 
            fcsr |= 0x10; // setting NV flag
          result = (f_rs1 == f_rs2) ? 1 : 0; 
          break;

        case Op::FLT_D:     
          if(std::isnan(f_rs1) || std::isnan(f_rs2)) 
            fcsr |= 0x10; // setting NV flag
          result = (f_rs1 <  f_rs2) ? 1 : 0; 
          break;

        case Op::FLE_D:     
          if(std::isnan(f_rs1) || std::isnan(f_rs2)) 
            fcsr |= 0x10; // setting NV flag
          result = (f_rs1 <= f_rs2) ? 1 : 0; 
          break;

        case Op::FCLASS_D:
          result = alu_classify_d(f_rs1); 
          break;

        // fp -> int
        case Op::FCVT_W_D:    result = fcvt_w_d(f_rs1); break;
        case Op::FCVT_WU_D:   result = fcvt_wu_d(f_rs1); break;
      }

      if(instr.op == Op::FCVT_W_D || instr.op == Op::FCVT_WU_D) {
        fp_end();
      }

      set_reg(instr.rd, result);
    } 

    else if(
      instr.op == Op::FADD_D    ||
      instr.op == Op::FSUB_D    ||
      instr.op == Op::FMUL_D    ||
      instr.op == Op::FDIV_D    ||
      instr.op == Op::FSQRT_D   ||
      instr.op == Op::FMIN_D    ||
      instr.op == Op::FMAX_D    ||
      instr.op == Op::FCVT_D_W  ||
      instr.op == Op::FCVT_D_WU ||
      instr.op == Op::FCVT_D_S
    ){
      double f_rs1 = std::bit_cast<double>(fp_rs1);
      double f_rs2 = std::bit_cast<double>(fp_rs2);
      double result;

      ROUNDING_MODE rm = ROUNDING_MODE::INV;

      if(instr.op != Op::FMIN_D && instr.op != Op::FMAX_D){
        rm = get_rm(instr);
        if(rm == ROUNDING_MODE::INV) return;
        fp_begin(rm);
      }
        

      switch(instr.op){
        default: Error<std::runtime_error>("Invalid op type (should not happen)");
        case Op::FADD_D:  result = f_rs1 + f_rs2; break;
        case Op::FSUB_D:  result = f_rs1 - f_rs2; break;
        case Op::FMUL_D:  result = f_rs1 * f_rs2; break;
        case Op::FDIV_D:  result = f_rs1 / f_rs2; break;
        case Op::FSQRT_D: result = sqrt(f_rs1); break;
        case Op::FMIN_D:  result = alu_fmin_d(f_rs1, f_rs2); break;
        case Op::FMAX_D:  result = alu_fmax_d(f_rs1, f_rs2); break;

        // int -> fp
        case Op::FCVT_D_W:  result = (double)((i32)rs1_value); break;
        case Op::FCVT_D_WU: result = (double)(rs1_value); break;

        // f -> d
        case Op::FCVT_D_S:  result = (double)(std::bit_cast<float>((u32)get_freg_s(instr.rs1))); break;

      }

      if(instr.op != Op::FMIN_D && instr.op != Op::FMAX_D){
        fp_end();
      }

      set_freg(instr.rd, std::bit_cast<u64>(canon_nan_d(result)));

    }
    else{
      Error<std::runtime_error>("Unhandled Operation");
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
    return UINT32_MAX;
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


u32 CPU::alu_classify_s(float v) {

  u32 bits = std::bit_cast<u32>(v);

  bool sign = bits >> 31;
  u8 exp    = (bits >> 23) & 0xFF;
  u32 mant  = bits & 0x007FFFFF;
  
  if(exp == 0xFF){
    if(mant == 0x0){
      return sign ? 0x001 : 0x080; // -inf / +inf 
    }else{
      return (mant & 0x400000) ? 0x200 : 0x100; // quite NaN / signaling NaN
    }
  }
  if(exp == 0x00){
    if(mant == 0x0){
      return sign ? 0x008 : 0x010; // -0.0 / +0.0
    }else{
      return sign ? 0x004 : 0x020; // neg. subnormal / pos. subnormal
    }
  }
  return sign ? 0x002 : 0x040; // neg. normal / pos. normal
  
}

i32 CPU::fcvt_w_s(float v) {

  // special cases
  if(std::isnan(v)) {
    fcsr |= FCSR_NV;
    return INT32_MAX;
  }

  if(v >= 9223372036854775808.0f) {
    fcsr |= 0x10;
    return INT32_MAX;
  }

  if(v <= -9223372036854775808.0f){
    fcsr |= 0x10;
    return INT32_MIN;
  }

  // round using current Rounding Mode, sets NX if in exact
  i64 r = llrintf(v);

  // test rounded value
  if(r > INT32_MAX || r < INT32_MIN){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX 
    fcsr |= 0x10; // set NV
    return (r > 0) ? INT32_MAX : INT32_MIN;
  }

  return (i32)(r);
}

u32 CPU::fcvt_wu_s(float v) {

  // special cases
  if(std::isnan(v)) {
    fcsr |= FCSR_NV;
    return UINT32_MAX;
  }

  if(v >= 9223372036854775808.0f) {
    fcsr |= 0x10;
    return UINT32_MAX;
  }
  
  if(v <= -9223372036854775808.0f) {
    fcsr |= 0x10;
    return 0;
  }
  
  // round using current Rounding Mode, sets NX if in exact
  i64 r = llrintf(v);

  // test rounded value
  if(r > (i64)UINT32_MAX){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX 
    fcsr |= 0x10; // set NV
    return UINT32_MAX;
  }

  if(r < 0){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX 
    fcsr |= 0x10; // set NV
    return 0;
  }

  return (u32)(r);
}



float CPU::alu_fmin_s(float a, float b) {
  
  if(is_snan_s(a) || is_snan_s(b)){
    fcsr |= 0x10; // setting NV/Invalid flag
  }

  if(isnan(a) && !isnan(b)) return b;
  if(!isnan(a) && isnan(b)) return a;
  if(isnan(a) && isnan(b)) return NAN;

  if(a == 0.0f && std::signbit(a) && b == 0.0f && !std::signbit(b)) return -0.0f;
  if(a == 0.0f && !std::signbit(a) && b == 0.0f && std::signbit(b)) return -0.0f;
  
  return std::fmin(a, b);
}

float CPU::alu_fmax_s(float a, float b) {

  if(is_snan_s(a) || is_snan_s(b)){
    fcsr |= 0x10; // setting NV/Invalid flag
  }

  if(isnan(a) && !isnan(b)) return b;
  if(!isnan(a) && isnan(b)) return a;
  if(isnan(a) && isnan(b)) return NAN;

  if(a == 0.0f && std::signbit(a) && b == 0.0f && !std::signbit(b)) return +0.0f;
  if(a == 0.0f && !std::signbit(a) && b == 0.0f && std::signbit(b)) return +0.0f;

  return std::fmax(a, b);
}

float CPU::canon_nan_s(float in) {
  if(std::isnan(in)) return std::bit_cast<float>(NAN_S);
  return in;
}

bool CPU::is_snan_s(float f) {
  return alu_classify_s(f) & 0x100;
}

u64 CPU::alu_classify_d(double v){

  u64 bits = std::bit_cast<u64>(v);

  bool sign = bits >> 63;
  u16  exp  = (bits >> 52) & 0x7FF;
  u64 mant  = bits & 0x000FFFFFFFFFFFFF;

  if(exp == 0x7FF){
    if(mant == 0x0){
      return sign ? 0x001 : 0x080; // -inf / +inf 
    }else{
      return (mant & 0x8000000000000) ? 0x200 : 0x100; // quite NaN / signaling NaN
    }
  }
  if(exp == 0x000){
    if(mant == 0x0){
      return sign ? 0x008 : 0x010; // -0.0 / +0.0
    }else{
      return sign ? 0x004 : 0x020; // neg. subnormal / pos. subnormal
    }
  }
  return sign ? 0x002 : 0x040; // neg. normal / pos. normal
}

i64 CPU::fcvt_w_d(double v) {

  if(std::isnan(v)){
    fcsr |= FCSR_NV;
    return INT32_MAX;
  }

  if(v >= 9223372036854775808.0) {
    fcsr |= 0x10;
    return INT32_MAX;
  }

  if(v <= -9223372036854775808.0){
    fcsr |= 0x10;
    return INT32_MIN;
  }

  i64 r = llrint(v);

  if(r > INT32_MAX || r < INT32_MIN){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX
    fcsr |= 0x10; // set NV
    return (r > 0) ? INT32_MAX : INT32_MIN;
  }

  return (i32)(r);
  
}

u64 CPU::fcvt_wu_d(double v) {
  
  if(std::isnan(v)){
    fcsr |= FCSR_NV;
    return UINT32_MAX;
  }

  if(v >= 9223372036854775808.0) {
    fcsr |= 0x10;
    return UINT32_MAX;
  }

  if(v <= -9223372036854775808.0){
    fcsr |= 0x10;
    return 0;
  }

  i64 r = llrint(v);

  if(r > (i64)UINT32_MAX){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX
    fcsr |= 0x10; // set NV
    return UINT32_MAX;
  }

  if(r < 0){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX
    fcsr |= 0x10; // set NV
    return 0;
  }

  return (u32)(r);
  
}

double CPU::alu_fmin_d(double a, double b) {

  if(is_snan_d(a) || is_snan_d(b)){
    fcsr |= 0x10; // setting NV/Invalid flag
  }

  if(isnan(a) && !isnan(b)) return b;
  if(!isnan(a) && isnan(b)) return a;
  if(isnan(a) && isnan(b)) return std::bit_cast<double>(NAN_D); // = canon NaN in Double prec.

  if(a == 0.0f && std::signbit(a) && b == 0.0f && !std::signbit(b)) return -0.0f;
  if(a == 0.0f && !std::signbit(a) && b == 0.0f && std::signbit(b)) return -0.0f;

  return std::min(a, b);
}

double CPU::alu_fmax_d(double a, double b) {

  if(is_snan_d(a) || is_snan_d(b)){
    fcsr |= 0x10; // setting NV/Invalid flag
  }

  if(isnan(a) && !isnan(b)) return b;
  if(!isnan(a) && isnan(b)) return a;
  if(isnan(a) && isnan(b)) return std::bit_cast<double>(NAN_D); // = canon NaN in Double prec.

  if(a == 0.0f && std::signbit(a) && b == 0.0f && !std::signbit(b)) return +0.0f;
  if(a == 0.0f && !std::signbit(a) && b == 0.0f && std::signbit(b)) return +0.0f;

  return std::max(a, b);
}

double CPU::canon_nan_d(double in) {
  if(std::isnan(in)) return std::bit_cast<double>(NAN_D);
  return in;
}

bool CPU::is_snan_d(double d) {
  return alu_classify_d(d) & 0x100;
}

void CPU::enter_exception(u32 addr, EXCEPTION_CODE code, u32 word){
  enter_trap(addr, (u32)code, word);
}

void CPU::enter_interrupt(u32 addr, INTERRUPT_CODE code){
  enter_trap(addr, (u32)code, 0); // interrupts always have tval = 0
}

void CPU::enter_trap(u32 trap_addr, u32 cause, u32 tval) {
  set_csr(CSR_ADDR::mepc, trap_addr);       // mepc <- addr of the ecall
  set_csr(CSR_ADDR::mcause, (u32)cause);    // 3: ebreak, 11: ecall Machine mode, 8: ecall User mode
  set_csr(CSR_ADDR::mtval, tval);

  u32 mstatus = get_csr(CSR_ADDR::mstatus);

  u32 mie = (mstatus >> 3) & 1;                         // aktuelles MIE
  mstatus = (mstatus & ~(MSTATUS_MPIE)) | (mie << 7);   // MPIE <- MIE
  mstatus &= ~(MSTATUS_MIE);                            // MIE <- 0 (handler läuft ungestört)
  mstatus |= (MSTATUS_MPP_MASK);                        // MPP <- 3 (aus m-mode)

  set_csr(CSR_ADDR::mstatus, mstatus);

  u32 mtvec = get_csr(CSR_ADDR::mtvec);

  u32 base = mtvec & ~0x3;
  u8  mode = mtvec & 0x3;

  if(mode == 0b00){
    // direct mode
    pc = base;
  }
  else if(mode == 0b01){
    // vectored mode
    if(cause & 0x80000000){ // = if interrupt (not exception)
      pc = base + 4 * (cause & 0x7FFFFFFF); // without int. bit i assume
    }else{
      pc = base; // exceptions work with the base
    }
  }
  // mode >= 2 should not happen anymore as in set_csr its not allowd for mtvec

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
