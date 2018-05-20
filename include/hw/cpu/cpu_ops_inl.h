#ifndef SDGBC_CPU_OPS_INL_H_
#define SDGBC_CPU_OPS_INL_H_

template <typename C, typename T>
void Cpu::ExecLoad(CpuRegister<C, T>& destReg, T val) {
  destReg.Set(val);
}

template <typename H, typename L>
void Cpu::ExecLoad(CpuRegister8Pair<H, L>& destRegPair, u16 val) {
  destRegPair.Set(val);
}

template <typename H, typename L>
void Cpu::ExecPop(CpuRegister8Pair<H, L>& destRegPair) {
  destRegPair.Set(ExecPop());
}

template <u8 L> // accept a u8 as it's technically $0000 + $XX
void Cpu::ExecRestart() {
  ExecPush(reg_.pc.Get()); // push current opcode addr
  reg_.pc.Set(L);
}

template <typename H, typename L>
void Cpu::ExecAdd16(CpuRegister8Pair<H, L>& destRegPair, u16 val) {
  const uint_fast32_t result = destRegPair.Get() + val;

  InternalDelay();
  reg_.f.SetNFlag(false);
  reg_.f.SetCFlag(result > 0xffff);
  reg_.f.SetHFlag((destRegPair.Get() & 0xfff) + (val & 0xfff) > 0xfff);
  destRegPair.Set(result & 0xffff);
}

template <typename C>
void Cpu::ExecInc(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecInc(destReg.Get()));
}

template <typename C>
void Cpu::ExecDec(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecDec(destReg.Get()));
}

template <typename H, typename L>
void Cpu::ExecInc16(CpuRegister8Pair<H, L>& destRegPair) {
  InternalDelay();
  ++destRegPair;
}

template <typename C>
void Cpu::ExecInc16(CpuRegister<C, u16>& destReg) {
  InternalDelay();
  ++destReg;
}

template <typename H, typename L>
void Cpu::ExecDec16(CpuRegister8Pair<H, L>& destRegPair) {
  InternalDelay();
  --destRegPair;
}

template <typename C>
void Cpu::ExecDec16(CpuRegister<C, u16>& destReg) {
  InternalDelay();
  --destReg;
}

template <typename C>
void Cpu::ExecRotLeft(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecRotLeft(destReg.Get()));
}

template <typename C>
void Cpu::ExecRotLeftThroughCarry(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecRotLeftThroughCarry(destReg.Get()));
}

template <typename C>
void Cpu::ExecRotRight(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecRotRight(destReg.Get()));
}

template <typename C>
void Cpu::ExecRotRightThroughCarry(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecRotRightThroughCarry(destReg.Get()));
}

template <typename C>
void Cpu::ExecShiftLeft(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecShiftLeft(destReg.Get()));
}

template <typename C>
void Cpu::ExecShiftRight(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecShiftRight(destReg.Get()));
}

template <typename C>
void Cpu::ExecShiftRightSigned(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecShiftRightSigned(destReg.Get()));
}

template <typename C>
void Cpu::ExecSwap(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecSwap(destReg.Get()));
}

template <unsigned int B>
void Cpu::ExecTestBit(u8 val) {
  static_assert(B < 8, "B must have a value from 0-7");
  reg_.f.SetZFlag(!((val >> B) & 1));
  reg_.f.SetNFlag(false);
  reg_.f.SetHFlag(true);
}

template <unsigned int B>
u8 Cpu::ExecSetBit(u8 val) {
  static_assert(B < 8, "B must have a value from 0-7");
  return val | (1 << B);
}

template <unsigned int B, typename C>
void Cpu::ExecSetBit(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecSetBit<B>(destReg.Get()));
}

template <unsigned int B>
void Cpu::ExecSetBit(u16 destLoc) {
  IoWrite8(destLoc, ExecSetBit<B>(IoRead8(destLoc)));
}

template <unsigned int B>
u8 Cpu::ExecResetBit(u8 val) {
  static_assert(B < 8, "B must have a value from 0-7");
  return val & ~(1 << B);
}

template <unsigned int B, typename C>
void Cpu::ExecResetBit(CpuRegister<C, u8>& destReg) {
  destReg.Set(ExecResetBit<B>(destReg.Get()));
}

template <unsigned int B>
void Cpu::ExecResetBit(u16 destLoc) {
  IoWrite8(destLoc, ExecResetBit<B>(IoRead8(destLoc)));
}

#endif // SDGBC_CPU_OPS_INL_H_
