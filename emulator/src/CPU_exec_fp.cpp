
#include <cstring>
#include <cfenv>

#include "CONFIG.hpp"
#include "DECODE.hpp"
#include "DEFS.hpp"
#include "CPU.hpp"

/// RV32F / RV32D
void CPU::exec_load_fp(){
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


void CPU::exec_store_fp(){  
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


void CPU::exec_fp_fused(){
  ROUNDING_MODE rm = get_rm();
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

void CPU::exec_fp_alu_single(){

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
      rm = get_rm();
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
      rm = get_rm();
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

void CPU::exec_fp_alu_double(){

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
      rm = get_rm();
      if(rm == ROUNDING_MODE::INV) return;
      fp_begin(rm);
    }
      

    switch(instr.op){
      default: Error<std::runtime_error>("Invalid op type (should not happen)");
      
      case Op::FEQ_D:     
        if(is_snan_d(f_rs1) || is_snan_d(f_rs2)) 
          fcsr |= FCSR_NV; // setting NV flag
        result = (f_rs1 == f_rs2) ? 1 : 0; 
        break;

      case Op::FLT_D:     
        if(std::isnan(f_rs1) || std::isnan(f_rs2)) 
          fcsr |= FCSR_NV; // setting NV flag
        result = (f_rs1 <  f_rs2) ? 1 : 0; 
        break;

      case Op::FLE_D:     
        if(std::isnan(f_rs1) || std::isnan(f_rs2)) 
          fcsr |= FCSR_NV; // setting NV flag
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
      rm = get_rm();
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


// Helper for RV32F/D ext.
ROUNDING_MODE CPU::get_rm() {

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
        

      switch(frm){
        default:
          enter_exception(instr.addr, EXCEPTION_CODE::ILLEGAL_INSTRUCTION, instr.word);
          return ROUNDING_MODE::INV;
        case 0b000: return ROUNDING_MODE::RNE;
        case 0b001: return ROUNDING_MODE::RTZ;
        case 0b010: return ROUNDING_MODE::RDN;
        case 0b011: return ROUNDING_MODE::RUP;
        case 0b100: return ROUNDING_MODE::RMM;
      }
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

u64 CPU::nanbox(u32 val) {
  return 0xFFFFFFFF00000000ULL | (u32)(val);
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
    fcsr |= FCSR_NV;
    return INT32_MAX;
  }

  if(v <= -9223372036854775808.0f){
    fcsr |= FCSR_NV;
    return INT32_MIN;
  }

  // round using current Rounding Mode, sets NX if in exact
  i64 r = llrintf(v);

  // test rounded value
  if(r > INT32_MAX || r < INT32_MIN){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX 
    fcsr |= FCSR_NV; // set NV
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
    fcsr |= FCSR_NV;
    return UINT32_MAX;
  }
  
  if(v <= -9223372036854775808.0f) {
    fcsr |= FCSR_NV;
    return 0;
  }
  
  // round using current Rounding Mode, sets NX if in exact
  i64 r = llrintf(v);

  // test rounded value
  if(r > (i64)UINT32_MAX){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX 
    fcsr |= FCSR_NV; // set NV
    return UINT32_MAX;
  }

  if(r < 0){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX 
    fcsr |= FCSR_NV; // set NV
    return 0;
  }

  return (u32)(r);
}



float CPU::alu_fmin_s(float a, float b) {
  
  if(is_snan_s(a) || is_snan_s(b)){
    fcsr |= FCSR_NV; // setting NV/Invalid flag
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
    fcsr |= FCSR_NV; // setting NV/Invalid flag
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
    fcsr |= FCSR_NV;
    return INT32_MAX;
  }

  if(v <= -9223372036854775808.0){
    fcsr |= FCSR_NV;
    return INT32_MIN;
  }

  i64 r = llrint(v);

  if(r > INT32_MAX || r < INT32_MIN){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX
    fcsr |= FCSR_NV; // set NV
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
    fcsr |= FCSR_NV;
    return UINT32_MAX;
  }

  if(v <= -9223372036854775808.0){
    fcsr |= FCSR_NV;
    return 0;
  }

  i64 r = llrint(v);

  if(r > (i64)UINT32_MAX){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX
    fcsr |= FCSR_NV; // set NV
    return UINT32_MAX;
  }

  if(r < 0){
    std::feclearexcept(FE_ALL_EXCEPT); // discard llrints NX
    fcsr |= FCSR_NV; // set NV
    return 0;
  }

  return (u32)(r);
  
}

double CPU::alu_fmin_d(double a, double b) {

  if(is_snan_d(a) || is_snan_d(b)){
    fcsr |= FCSR_NV; // setting NV/Invalid flag
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
    fcsr |= FCSR_NV; // setting NV/Invalid flag
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
