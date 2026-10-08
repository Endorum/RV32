#include <cstring>
#include <cfenv>

#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include "CPU.hpp"

u32 CPU::mask(u16 idx) {
  CSR_ADDR addr = static_cast<CSR_ADDR>(idx);

  switch(addr){
    default: return 0xFFFFFFFF;
    
    // read only csr's
    case CSR_ADDR::misa:    return 0x00000000;
    case CSR_ADDR::tselect: return 0x00000000;
    case CSR_ADDR::time:    return 0x00000000;
    case CSR_ADDR::timeh:   return 0x00000000;

    case CSR_ADDR::mstatus: 
      return MSTATUS_MIE | MSTATUS_MPIE | MSTATUS_FS_MASK | MSTATUS_MPP_MASK;


  }
}

void CPU::set_csr(u16 idx, u32 val) {
  const u32 m = mask(idx);

  if(idx == (u16)(CSR_ADDR::fflags)){
    csr[(u16)CSR_ADDR::mstatus] |= MSTATUS_FS_DIRTY; // set FS = 11 = dirty
    fcsr = (fcsr & ~0x1F) | (val & 0x1F);
  }
  else if(idx == (u16)(CSR_ADDR::frm)){
    csr[(u16)CSR_ADDR::mstatus] |= MSTATUS_FS_DIRTY; // set FS = 11 = dirty
    fcsr = (fcsr & ~0xE0) | ((val & 0x7) << 5);
  }
  else if(idx == (u16)(CSR_ADDR::fcsr)){
    csr[(u16)CSR_ADDR::mstatus] |= MSTATUS_FS_DIRTY; // set FS = 11 = dirty
    fcsr = (fcsr & ~0xFF) | (val & 0xFF);
  }
  else if(idx == (u16)(CSR_ADDR::mtvec)){

    if((val & 0x3) >= 2){ // invalid / unhandled interrup modes
      csr[idx] = (val & ~0x3); // write mode = 0 instead
    }else{
      csr[idx] = val;
    }

  }else if(idx == (u16)(CSR_ADDR::mstatus)){
    // if MPP is = 2 -> invalid, keep old one
    if(((u32)(val & MSTATUS_MPP_MASK) >> 11) == 0b10){
      val = (val & ~MSTATUS_MPP_MASK);
      csr[idx] = (val & m) | (csr[idx] & ~m);  
    }
    else{
      csr[idx] = (val & m) | (csr[idx] & ~m);  
    }
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
  }else if(idx == (u16)(CSR_ADDR::time)){
    return clint->get_time_lo();
  }else if(idx == (u16)(CSR_ADDR::timeh)){
    return clint->get_time_hi();
  }

  else{
    return csr[idx];
  }
}



