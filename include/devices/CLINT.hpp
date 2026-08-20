#ifndef CLINT_HPP
#define CLINT_HPP

#include "DEVICE.hpp"
#include "CONFIG.hpp"
#include "DEFS.hpp"
#include "UTILS.hpp"

#define MSIP_ADDR     0x0000
#define MTIMECMP_ADDR 0x4000
#define MTIME_ADDR    0xBFF8

class CLINT : public Device {
public:
  explicit CLINT(u32 start, u32 size) : Device(start, size, "CLINT"){
    mtime     = 0;
    msip      = 0;
    mtimecmp  = 0xFFFFFFFFFFFFFFFF;
  }

  u32 load(u32 addr, u8 size) override;

  void store(u32 addr, u8 size, u32 value) override;

  void tick();

  bool mtip() const { return mtime >= mtimecmp; }
  bool msip_pending() const { return msip & 1; }

  u32 pending_mip() const {
    return 
      (mtip() ? MIP_MTIP : 0) | 
      (msip_pending() ? MIP_MSIP : 0);
  }

private:
  u32 msip;
  u64 mtimecmp;
  u64 mtime;

};

#endif // CLINT_HPP