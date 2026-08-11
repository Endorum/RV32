
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

  // start at reset vector (start of ROM / firmware as of right now)
  pc = config.reset_vector;
  cycle = 0;
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


  // 0x6F = JAL +0 = infinite loop
  if(word == 0x0000006F) {
    std::cout << state_str() << std::endl; 
    exit(0);
  }
  
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

void CPU::set_csr(u16 idx, u32 val) {
  csr[idx] = val;
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

void CPU::execute(const Instruction& instr){

  if(instr.op == Op::INVALID) Error<std::runtime_error>("Op was of type INVALID");
  
  if(instr.type == BaseType::ALU_R || instr.type == BaseType::ALU_I){
    u32 result = 0;
    u32 A = rs1_value;
    u32 B = (instr.type == BaseType::ALU_I) ? (u32)instr.imm : rs2_value;

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
    u32 addr  = rs1_value + (u32)instr.imm;

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
    u32 addr = rs1_value + instr.imm;

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");

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

    if(branch)
      pc = instr.addr + instr.imm;

  }


  else if(instr.op == Op::JAL){
    set_reg(instr.rd, instr.addr + 4);

    pc = instr.addr + instr.imm;
  }
  
  else if(instr.op == Op::JALR){
    set_reg(instr.rd, instr.addr + 4);

    pc = (((i32)rs1_value + instr.imm) & ~1);
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
      enter_trap(instr.addr, 11, 0);
    }
    
    else if(instr.op == Op::EBREAK){
      enter_trap(instr.addr, 3, instr.addr);
    }

    else if(instr.op == Op::WFI){
      // wfi = wait for interrup = do nothing for now
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

void CPU::enter_trap(u32 trap_addr, u32 cause, u32 tval) {
  set_csr(CSR_ADDR::mepc, trap_addr);  // mepc <- addr of the ecall
  set_csr(CSR_ADDR::mcause, cause);   // 3: ebreak, 11: ecall Machine mode, 8: ecall User mode
  set_csr(CSR_ADDR::mtval, tval);

  u32 mstatus = get_csr(CSR_ADDR::mstatus);

  u32 mie = (mstatus >> 3) & 1;                         // aktuelles MIE
  mstatus = (mstatus & ~(MSTATUS_MPIE)) | (mie << 7);   // MPIE <- MIE
  mstatus &= ~(MSTATUS_MIE);                            // MIE <- 0 (handler läuft ungestört)
  mstatus |= (MSTATUS_MPP);                             // MPP <- 3 (aus m-mode)

  set_csr(CSR_ADDR::mstatus, mstatus);

  u32 mtvec = get_csr(CSR_ADDR::mtvec);
  pc = mtvec & ~0x3;
}

void CPU::mret() {
  u32 mstatus = get_csr(CSR_ADDR::mstatus);
  
  u32 mpie = (mstatus >> 7) & 1;                      // MIE herauslesen
  mstatus = (mstatus & ~(MSTATUS_MIE)) | (mpie << 3); // MIE <- MPIE wiederherstellen
  mstatus |= (MSTATUS_MPIE);                          // MPIE <- 1

  set_csr(CSR_ADDR::mstatus, mstatus);

  pc = get_csr(CSR_ADDR::mepc) & ~0x1;
  
}
