#include "hw/cpu/cpu_reg.h"

CpuFlagRegister::CpuFlagRegister() : val_(0) {}

CpuFlagRegister::CpuFlagRegister(u8 val) {
  SetImpl(val);
}

void CpuFlagRegister::SetImpl(u8 val) {
  val_ = val & 0xf0; // ensure bits 0-3 are always 0
}

u8 CpuFlagRegister::GetImpl() const {
  return val_;
}

void CpuFlagRegister::SetZFlag(bool val) {
  Set(val ? val_ | kCpuFlagRegisterZMask : val_ & ~kCpuFlagRegisterZMask);
}

void CpuFlagRegister::SetNFlag(bool val) {
  Set(val ? val_ | kCpuFlagRegisterNMask : val_ & ~kCpuFlagRegisterNMask);
}

void CpuFlagRegister::SetHFlag(bool val) {
  Set(val ? val_ | kCpuFlagRegisterHMask : val_ & ~kCpuFlagRegisterHMask);
}

void CpuFlagRegister::SetCFlag(bool val) {
  Set(val ? val_ | kCpuFlagRegisterCMask : val_ & ~kCpuFlagRegisterCMask);
}

bool CpuFlagRegister::GetZFlag() const {
  return (val_ & kCpuFlagRegisterZMask) != 0;
}

bool CpuFlagRegister::GetNFlag() const {
  return (val_ & kCpuFlagRegisterNMask) != 0;
}

bool CpuFlagRegister::GetHFlag() const {
  return (val_ & kCpuFlagRegisterHMask) != 0;
}

bool CpuFlagRegister::GetCFlag() const {
  return (val_ & kCpuFlagRegisterCMask) != 0;
}

CpuRegisters::CpuRegisters() : af(a, f), bc(b, c), de(d, e), hl(h, l) {}

