#ifndef SDGBC_CPU_REG_H_
#define SDGBC_CPU_REG_H_

#include "hw/cpu/cpu_reg_base.h"

// masks of the flag bits within register F
enum CpuFlagRegisterMask : u8 {
  kCpuFlagRegisterCMask = 0x10, // carry flag
  kCpuFlagRegisterHMask = 0x20, // half carry flag
  kCpuFlagRegisterNMask = 0x40, // add/sub flag
  kCpuFlagRegisterZMask = 0x80  // zero flag
};

class CpuFlagRegister : public CpuRegister<CpuFlagRegister, u8> {
public:
  CpuFlagRegister();
  explicit CpuFlagRegister(u8 val);

  void SetImpl(u8 val);
  u8 GetImpl() const;

  void SetZFlag(bool val);
  bool GetZFlag() const;

  void SetNFlag(bool val);
  bool GetNFlag() const;

  void SetHFlag(bool val);
  bool GetHFlag() const;

  void SetCFlag(bool val);
  bool GetCFlag() const;

private:
  u8 val_;
};

using CpuRegisterF      = CpuFlagRegister;
using CpuRegisterAFPair = CpuRegister8Pair<CpuRegisterBasic8, CpuRegisterF>;

struct CpuRegisters {
  CpuRegisterBasic16 pc, sp;
  CpuRegisterBasic8 a, b, c, d, e, h, l;
  CpuRegisterF f;

  CpuRegisterBasic8Pair bc, de, hl;
  CpuRegisterAFPair af;

  CpuRegisters();
};

#endif // SDGBC_CPU_REG_H_
