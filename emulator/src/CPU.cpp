
#include <cstring>
#include <cfenv>

#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include "CPU.hpp"
#include "ERROR.hpp"




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
    // clear instr
    instr = {};

    // machine starts in machine mode
    priv = PRIV_LEVEL::MACHINE;


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

    if(((get_csr(CSR_ADDR::mstatus) & MSTATUS_MIE) || (priv < PRIV_LEVEL::MACHINE) ) && pending){

        // software (Bit 3) has priority before timer (Bit 7)
        if(pending & MIP_MSIP)      enter_interrupt(pc, INTERRUPT_CODE::MACH_SOFTWARE_INT);
        else if(pending & MIP_MTIP) enter_interrupt(pc, INTERRUPT_CODE::MACH_TIMER_INT);

        return;
    }

    // fetch...
    u32 word = 0;
    try{
        word = load(pc, WORD);
    }
    catch (const TrapException& exc){
        if(exc.get_code() == EXCEPTION_CODE::LOAD_ACCESS_FAULT){
            enter_exception(pc, EXCEPTION_CODE::INSTR_ACCESS_FAULT, exc.get_tval());
        }else{
            enter_exception(pc, exc.get_code(), word);
        }
        return;
    
    }

  

    // ...decode...
    instr = decode(word, pc);

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
    try{
        execute();
    }
    catch (const TrapException& exc){
        if(exc.get_tval_addr()){
            // a memory / address faulty exception
            enter_exception(instr.addr, exc.get_code(), exc.get_tval());
        }else{
            // a "normal" exception
            enter_exception(instr.addr, exc.get_code(), instr.word);
        }
    
        return;
    }

    // if a TrapException has been thrown, enter exception
    halt_if_deadlock(); 
  
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
    out += std::format("\tmstatus: {:08X}, mtvec: {:08X}, mepc: {:08X}, mcause: {:08X}, mtval: {:08X}, mscratch: {:08X}\n", 
            get_csr(CSR_ADDR::mstatus), get_csr(CSR_ADDR::mtvec), get_csr(CSR_ADDR::mepc), get_csr(CSR_ADDR::mcause), get_csr(CSR_ADDR::mtval), get_csr(CSR_ADDR::mscratch));

    std::string priv_str[] = {"USER", "SUPERVISOR", "RESERVED", "MACHINE"};
    out += std::format("Priveledge level: {}\n", priv_str[(u8)priv]);

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

void CPU::set_mpp_priv(PRIV_LEVEL level) {
    set_csr_raw(CSR_ADDR::mstatus, (get_csr(CSR_ADDR::mstatus) & ~(MSTATUS_MPP_MASK)) | (((u32)level) << 11) );
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




void CPU::halt_if_deadlock() {
  
    // if word == 6F == j +0 == inf. loop AND interrupts are ->disabled<- then halt
    if(
        instr.word == 0x6F && 
        priv == PRIV_LEVEL::MACHINE &&
        ((get_csr(CSR_ADDR::mstatus) & MSTATUS_MIE)) == 0) {
        halted = true;
    }

}




void CPU::execute(){

    if(instr.op == Op::INVALID){
        enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
        return;
    }
  
    if(instr.type == BaseType::ALU_R || instr.type == BaseType::ALU_I){
        exec_alu();
    }


    else if(instr.type == BaseType::LOAD){
        exec_load();
    }


    else if(instr.type == BaseType::STORE){
        exec_store();
    }


    else if(instr.type == BaseType::BRANCH){
        exec_branch();
    }

    else if(instr.op == Op::JAL){
        exec_jal();
    }
  
    else if(instr.op == Op::JALR){
        exec_jalr();
    }
  
    else if(instr.op == Op::LUI){
        exec_lui();
    }
  
    else if(instr.op == Op::AUIPC){
        exec_auipc();
    }
  
    else if(instr.type == BaseType::FENCE){
        exec_fence();
    }
  
    else if(instr.type == BaseType::SYSTEM){
        exec_system();
    }

    /* RV32A */
    else if(instr.type == BaseType::ATOMIC){
        exec_atomic();
    }

    /* RV32F/D/H/Q */
    else if(instr.type == BaseType::LOAD_FP){
        exec_load_fp();
    }

    else if(instr.type == BaseType::STORE_FP){
        exec_store_fp();
    }

    /* RV32F */
    else if(instr.type == BaseType::FMADD || instr.type == BaseType::FMSUB || instr.type == BaseType::FNMSUB || instr.type == BaseType::FNMADD){
        exec_fp_fused();
    }

    else if(instr.type == BaseType::FP_ALU && instr.width == PREC_SINGLE){
        exec_fp_alu_single();
    }

    /* RV32D */
    else if(instr.type == BaseType::FP_ALU && instr.width == PREC_DOUBLE){
        exec_fp_alu_double();
    }

    else{
        enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
    }

}


void CPU::enter_exception(u32 addr, EXCEPTION_CODE code, u32 word){
    enter_trap(addr, (u32)code, word);
}       

void CPU::enter_interrupt(u32 addr, INTERRUPT_CODE code){
    enter_trap(addr, (u32)code, 0); // interrupts always have tval = 0
}

void CPU::enter_trap(u32 trap_addr, u32 cause, u32 tval) {
    set_csr_raw(CSR_ADDR::mepc, trap_addr);       // mepc <- addr of the ecall
    set_csr_raw(CSR_ADDR::mcause, (u32)cause);    // 
    set_csr_raw(CSR_ADDR::mtval, tval);

    u32 mstatus = get_csr(CSR_ADDR::mstatus);

    u32 mie = (mstatus >> 3) & 1;                         // aktuelles MIE
    mstatus = (mstatus & ~(MSTATUS_MPIE)) | (mie << 7);   // MPIE <- MIE
    mstatus &= ~(MSTATUS_MIE);                            // MIE <- 0 (handler läuft ungestört)
    mstatus = (mstatus & ~(MSTATUS_MPP_MASK)) | (((u32)priv) << 11);  // MPP <- save current priv level
    set_csr_raw(CSR_ADDR::mstatus, mstatus);

    // change priv level to machine for trap
    priv = PRIV_LEVEL::MACHINE;

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
    else{ // mode >= 2 should not happen anymore as in set_csr its not allowd for mtvec
        Error<std::runtime_error>("reserved trap modes");
        // throw TrapException(EXCEPTION_CODE::ILLEGAL_INSTRUCTION);
    }

    // why?
    reservation_valid = false;
}

void CPU::mret() {
    if(priv < PRIV_LEVEL::MACHINE){
        enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
        return;
    }

    u32 mstatus = get_csr(CSR_ADDR::mstatus);

    // get saved priv level bits and change priv level back to the old one
    u8 pl = (mstatus & MSTATUS_MPP_MASK) >> 11;

    if((PRIV_LEVEL)pl == PRIV_LEVEL::RESERVED){
        enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
        return;
    }

    priv = (PRIV_LEVEL)(pl);
  
    u32 mpie = (mstatus >> 7) & 1;                      // MIE herauslesen
    mstatus = (mstatus & ~(MSTATUS_MIE)) | (mpie << 3); // MIE <- MPIE wiederherstellen
    mstatus |= (MSTATUS_MPIE);                          // MPIE <- 1
    mstatus = (mstatus & ~(MSTATUS_MPP_MASK)) | (MSTATUS_MPP_U); // set U mode in mpp (do nothing because u=0)

  

  
  

    set_csr_raw(CSR_ADDR::mstatus, mstatus);

    pc = get_csr(CSR_ADDR::mepc) & ~0x1;
  
}

CPU_STATE CPU::get_state() {
        CPU_STATE state;
    
        for(int i=0;i<32;i++){
                state.regfile[i] = regfile[i];
                state.fregfile[i] = regfile[i];
        }
    
        for(int i=0;i<4096;i++){
                state.csr[i] = csr[i];
        }
    
        state.cycle = get_cycle();
    
        state.pc = pc;
    
        state.fcsr = fcsr;
    
        state.priv = priv;
    
        state.current_dis = get_current_dis();
    
        return state;
}

