#include "hw/cpu/cpu.h"
#include "hw/memory.h"
#include "util.h"

constexpr auto kSpeedSwitchTotalCycles = 130992u;

void Cpu::ExecOpStop0x10() {
  if (speedSwitchRequested_) {
    speedSwitchCyclesLeft_ = kSpeedSwitchTotalCycles;
  }

  status_ = CpuStatus::Stopped;
}

void Cpu::ExecOpHalt0x76() {
  status_ = CpuStatus::Halted;
}

void Cpu::ExecOpDi0xf3() {
  intme_ = false;
}

void Cpu::ExecOpEi0xfb() {
  intme_ = true;
}

void Cpu::ExecLoad(u16 destLoc, u8 val) {
  IoWrite8(destLoc, val);
}

void Cpu::ExecLoad(u16 destLoc, u16 val) {
  IoWrite8(destLoc, util::GetLo8(val));
  IoWrite8(destLoc + 1, util::GetHi8(val));
}

void Cpu::ExecOpLdd0x3a() {
  ExecLoad(reg_.a, IoRead8(reg_.hl--));
}

void Cpu::ExecOpLdd0x32() {
  ExecLoad(reg_.hl--, reg_.a.Get());
}

void Cpu::ExecOpLdi0x2a() {
  ExecLoad(reg_.a, IoRead8(reg_.hl++));
}

void Cpu::ExecOpLdi0x22() {
  ExecLoad(reg_.hl++, reg_.a.Get());
}

void Cpu::ExecOpLdhl0xf8() {
  const u8 val = IoPcReadNext8();

  InternalDelay();
  reg_.f.SetZFlag(false);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag((reg_.sp.Get() & 0xff) + val > 0xff);
  reg_.f.SetHFlag((reg_.sp.Get() & 0xf) + (val & 0xf) > 0xf);
  reg_.hl.Set(reg_.sp.Get() + static_cast<i8>(val));
}

void Cpu::ExecOpLd0xf9() {
  InternalDelay();
  ExecLoad(reg_.sp, reg_.hl.Get());
}

void Cpu::ExecPush(u16 val) {
  InternalDelay();
  IoWrite8((--reg_.sp).Get(), util::GetHi8(val));
  IoWrite8((--reg_.sp).Get(), util::GetLo8(val));
}

u16 Cpu::ExecPop() {
  const u8 lo = IoRead8(reg_.sp++);
  const u8 hi = IoRead8(reg_.sp++);
  return util::To16(hi, lo);
}

void Cpu::ExecJump(u16 targetLoc, bool shouldJump) {
  if (shouldJump) {
    InternalDelay();
    reg_.pc.Set(targetLoc);
  }
}

void Cpu::ExecCall(u16 targetLoc, bool shouldCall) {
  if (shouldCall) {
    ExecPush(reg_.pc.Get()); // push pc (points to next op)
    reg_.pc.Set(targetLoc);
  }
}

void Cpu::ExecReturn() {
  InternalDelay();
  reg_.pc.Set(ExecPop());
}

void Cpu::ExecReturn(bool shouldReturn) {
  InternalDelay();
  if (shouldReturn) {
    ExecReturn();
  }
}

void Cpu::ExecOpJp0xe9() {
  reg_.pc.Set(reg_.hl.Get());
}

void Cpu::ExecOpJr0x18() {
  const i8 val = static_cast<i8>(IoPcReadNext8());
  ExecJump(reg_.pc.Get() + val);
}

void Cpu::ExecOpJr0x20() {
  const i8 val = static_cast<i8>(IoPcReadNext8());
  ExecJump(reg_.pc.Get() + val, !reg_.f.GetZFlag());
}

void Cpu::ExecOpJr0x28() {
  const i8 val = static_cast<i8>(IoPcReadNext8());
  ExecJump(reg_.pc.Get() + val, reg_.f.GetZFlag());
}

void Cpu::ExecOpJr0x30() {
  const i8 val = static_cast<i8>(IoPcReadNext8());
  ExecJump(reg_.pc.Get() + val, !reg_.f.GetCFlag());
}

void Cpu::ExecOpJr0x38() {
  const i8 val = static_cast<i8>(IoPcReadNext8());
  ExecJump(reg_.pc.Get() + val, reg_.f.GetCFlag());
}

void Cpu::ExecOpReti0xd9() {
  ExecReturn();
  intme_ = true;
}

void Cpu::ExecAdd(u8 val) {
  const uint_fast16_t result = reg_.a.Get() + val;

  reg_.f.SetZFlag((result & 0xff) == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(result > 0xff);
  reg_.f.SetHFlag((reg_.a.Get() & 0xf) + (val & 0xf) > 0xf);
  reg_.a.Set(result & 0xff);
}

void Cpu::ExecAddWithCarry(u8 val) {
  const uint_fast8_t carryAdd = reg_.f.GetCFlag() ? 1 : 0;
  const uint_fast16_t result = reg_.a.Get() + val + carryAdd;

  reg_.f.SetZFlag((result & 0xff) == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(result > 0xff);
  reg_.f.SetHFlag((reg_.a.Get() & 0xf) + (val & 0xf) + carryAdd > 0xf);
  reg_.a.Set(result & 0xff);
}

void Cpu::ExecSub(u8 val) {
  const uint_fast16_t result = reg_.a.Get() - val;

  reg_.f.SetZFlag((result & 0xff) == 0);
  reg_.f.SetNFlag(true);
  reg_.f.SetCFlag(result > 0xff);
  reg_.f.SetHFlag((reg_.a.Get() & 0xf) < (val & 0xf));
  reg_.a.Set(result & 0xff);
}

void Cpu::ExecSubWithCarry(u8 val) {
  const uint_fast8_t carrySub = reg_.f.GetCFlag() ? 1 : 0;
  const uint_fast16_t result = reg_.a.Get() - val - carrySub;

  reg_.f.SetZFlag((result & 0xff) == 0);
  reg_.f.SetNFlag(true);
  reg_.f.SetCFlag(result > 0xff);
  reg_.f.SetHFlag((reg_.a.Get() & 0xf) < (val & 0xf) + carrySub);
  reg_.a.Set(result & 0xff);
}

void Cpu::ExecCompare(u8 val) {
  const uint_fast16_t result = reg_.a.Get() - val;

  reg_.f.SetZFlag((result & 0xff) == 0);
  reg_.f.SetNFlag(true);
  reg_.f.SetCFlag(result > 0xff);
  reg_.f.SetHFlag((reg_.a.Get() & 0xf) < (val & 0xf));
}

void Cpu::ExecAnd(u8 val) {
  const u8 result = reg_.a.Get() & val;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(false);
  reg_.f.SetHFlag(true);
  reg_.a.Set(result);
}

void Cpu::ExecOr(u8 val) {
  const u8 result = reg_.a.Get() | val;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(false);
  reg_.f.SetHFlag(false);
  reg_.a.Set(result);
}

void Cpu::ExecXor(u8 val) {
  const u8 result = reg_.a.Get() ^ val;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(false);
  reg_.f.SetHFlag(false);
  reg_.a.Set(result);
}

u8 Cpu::ExecInc(u8 val) {
  const u8 result = val + 1;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetHFlag((val & 0xf) + 1 > 0xf);
  return result;
}

void Cpu::ExecInc(u16 destLoc) {
  IoWrite8(destLoc, ExecInc(IoRead8(destLoc)));
}

u8 Cpu::ExecDec(u8 val) {
  const u8 result = val - 1;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(true);
  reg_.f.SetHFlag((val & 0xf) == 0);
  return result;
}

void Cpu::ExecDec(u16 destLoc) {
  IoWrite8(destLoc, ExecDec(IoRead8(destLoc)));
}

void Cpu::ExecOpAdd0xe8() {
  const u8 val = IoPcReadNext8();

  InternalDelay(2);
  reg_.f.SetZFlag(false);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag((reg_.sp.Get() & 0xff) + val > 0xff);
  reg_.f.SetHFlag((reg_.sp.Get() & 0xf) + (val & 0xf) > 0xf);
  reg_.sp.Set(reg_.sp.Get() + static_cast<i8>(val));
}

void Cpu::ExecOpDaa0x27() {
  // adjust A (and flag C) to allow for BCD addition and subtraction
  // NOTE: based on AWJ's very nice implementation of DAA @
  // https://forums.nesdev.com/viewtopic.php?t=15944#p196282
  u8 result = reg_.a.Get();

  if (reg_.f.GetNFlag()) {
    if (reg_.f.GetCFlag()) {
      result -= 0x60;
    }

    if (reg_.f.GetHFlag()) {
      result -= 6;
    }
  } else {
    if (reg_.f.GetCFlag() || result > 0x99) {
      result += 0x60;
      reg_.f.SetCFlag(true);
    }

    if (reg_.f.GetHFlag() || (result & 0xf) > 9) {
      result += 6;
    }
  }

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetHFlag(false);
  reg_.a.Set(result);
}

void Cpu::ExecOpCpl0x2f() {
  reg_.f.SetNFlag(true);
  reg_.f.SetHFlag(true);
  reg_.a.Set(~reg_.a.Get());
}

void Cpu::ExecOpCcf0x3f() {
  reg_.f.SetNFlag(false);
  reg_.f.SetHFlag(false);
  reg_.f.SetCFlag(!reg_.f.GetCFlag());
}

void Cpu::ExecOpScf0x37() {
  reg_.f.SetNFlag(false);
  reg_.f.SetHFlag(false);
  reg_.f.SetCFlag(true);
}

void Cpu::ExecOpCb0xcb() {
  ExecuteExOp(IoPcReadNext8());
}

u8 Cpu::ExecRotLeft(u8 val) {
  const u8 result = val << 1 | (val & 0x80) >> 7;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag((val & 0x80) != 0);
  reg_.f.SetHFlag(false);
  return result;
}

void Cpu::ExecRotLeft(u16 destLoc) {
  IoWrite8(destLoc, ExecRotLeft(IoRead8(destLoc)));
}

u8 Cpu::ExecRotLeftThroughCarry(u8 val) {
  const u8 result = val << 1 | (reg_.f.GetCFlag() ? 1 : 0);

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag((val & 0x80) != 0);
  reg_.f.SetHFlag(false);
  return result;
}

void Cpu::ExecRotLeftThroughCarry(u16 destLoc) {
  IoWrite8(destLoc, ExecRotLeftThroughCarry(IoRead8(destLoc)));
}

u8 Cpu::ExecRotRight(u8 val) {
  const u8 result = val >> 1 | (val & 1) << 7;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(val & 1);
  reg_.f.SetHFlag(false);
  return result;
}

void Cpu::ExecRotRight(u16 destLoc) {
  IoWrite8(destLoc, ExecRotRight(IoRead8(destLoc)));
}

u8 Cpu::ExecRotRightThroughCarry(u8 val) {
  const u8 result = val >> 1 | (reg_.f.GetCFlag() ? 0x80 : 0);

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(val & 1);
  reg_.f.SetHFlag(false);
  return result;
}

void Cpu::ExecRotRightThroughCarry(u16 destLoc) {
  IoWrite8(destLoc, ExecRotRightThroughCarry(IoRead8(destLoc)));
}

void Cpu::ExecOpRlca0x07() {
  ExecRotLeft(reg_.a);
  reg_.f.SetZFlag(false);
}

void Cpu::ExecOpRla0x17() {
  ExecRotLeftThroughCarry(reg_.a);
  reg_.f.SetZFlag(false);
}

void Cpu::ExecOpRrca0x0f() {
  ExecRotRight(reg_.a);
  reg_.f.SetZFlag(false);
}

void Cpu::ExecOpRra0x1f() {
  ExecRotRightThroughCarry(reg_.a);
  reg_.f.SetZFlag(false);
}

u8 Cpu::ExecShiftLeft(u8 val) {
  const u8 result = val << 1;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag((val & 0x80) != 0);
  reg_.f.SetHFlag(false);
  return result;
}

void Cpu::ExecShiftLeft(u16 destLoc) {
  IoWrite8(destLoc, ExecShiftLeft(IoRead8(destLoc)));
}

u8 Cpu::ExecShiftRight(u8 val) {
  const u8 result = val >> 1;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(val & 1);
  reg_.f.SetHFlag(false);
  return result;
}

void Cpu::ExecShiftRight(u16 destLoc) {
  IoWrite8(destLoc, ExecShiftRight(IoRead8(destLoc)));
}

u8 Cpu::ExecShiftRightSigned(u8 val) {
  // we preserve the msb; essentially does a sign extend
  const u8 result = val >> 1 | (val & 0x80);

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(val & 1);
  reg_.f.SetHFlag(false);
  return result;
}

void Cpu::ExecShiftRightSigned(u16 destLoc) {
  IoWrite8(destLoc, ExecShiftRightSigned(IoRead8(destLoc)));
}

u8 Cpu::ExecSwap(u8 val) {
  const u8 result = (val & 0x0f) << 4 | (val & 0xf0) >> 4;

  reg_.f.SetZFlag(result == 0);
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(false);
  reg_.f.SetHFlag(false);
  return result;
}

void Cpu::ExecSwap(u16 destLoc) {
  IoWrite8(destLoc, ExecSwap(IoRead8(destLoc)));
}
