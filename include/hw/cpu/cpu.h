#ifndef SDGBC_CPU_H_
#define SDGBC_CPU_H_

#include "hw/cpu/cpu_reg.h"

enum class CpuStatus {
  Running,
  Halted,
  Stopped,
  Hung
};

enum CpuInterruptMask : u8 {
  kCpuInterrupt0x40 = 0x01,
  kCpuInterrupt0x48 = 0x02,
  kCpuInterrupt0x50 = 0x04,
  kCpuInterrupt0x58 = 0x08,
  kCpuInterrupt0x60 = 0x10
};

class Mmu;
class Dma;
class Joypad;

class Cpu {
public:
  explicit Cpu(Mmu& mmu, const Dma& dma, const Joypad& joypad);

  void Reset(bool cgbMode);
  bool Resume(); // resumes the CPU if it was halted

  unsigned int Update(); // returns the amount of CPU clock cycles spent

  const CpuRegisters& GetRegisters() const;
  CpuStatus GetStatus() const;

  bool GetIntme() const;

  void SetInte(u8 val);
  u8 GetInte() const;

  void IntfRequest(u8 mask);
  void SetIntf(u8 val);
  u8 GetIntf() const;

  void SetKey1(u8 val);
  u8 GetKey1() const;

  bool IsInCgbMode() const;

  bool IsInDoubleSpeedMode() const;
  bool IsSpeedSwitchInProgress() const;

private:
  Mmu& mmu_;
  const Dma& dma_;
  const Joypad& joypad_;

  CpuStatus status_;
  bool cgbMode_;
  unsigned int updateCycles_;

  bool speedSwitchRequested_, doubleSpeedMode_;
  unsigned int speedSwitchCyclesLeft_;

  // general & interrupt registers
  CpuRegisters reg_;
  bool intme_;
  u8 intf_, inte_;

  void HandleStoppedUpdate();

  bool HandleInterrupts(); // returns whether or not an int was serviced
  template <u8 L>
  void ServiceInterrupt() {
    intme_ = false;
    InternalDelay(2);
    ExecPush(reg_.pc.Get());
    reg_.pc.Set(L);
  }

  bool ExecuteOp(u8 op);
  void ExecuteExOp(u8 exOp);

  void InternalDelay(unsigned int numAccesses = 1);

  u8 IoRead8(u16 loc);
  void IoWrite8(u16 loc, u8 val);
  u8 IoPcReadNext8();
  u16 IoPcReadNext16(); // NOTE: reads in little-endian

  void ExecOpStop0x10();
  void ExecOpHalt0x76();

  void ExecOpDi0xf3();
  void ExecOpEi0xfb();

  void ExecLoad(u16 destLoc, u8 val);
  void ExecLoad(u16 destLoc, u16 val);
  template <typename C, typename T>
  void ExecLoad(CpuRegister<C, T>& destReg, T val);
  template <typename H, typename L>
  void ExecLoad(CpuRegister8Pair<H, L>& destRegPair, u16 val);
  void ExecOpLdi0x22();
  void ExecOpLdi0x2a();
  void ExecOpLdd0x32();
  void ExecOpLdd0x3a();
  void ExecOpLdhl0xf8();
  void ExecOpLd0xf9();

  void ExecPush(u16 val);
  u16 ExecPop();
  template <typename H, typename L>
  void ExecPop(CpuRegister8Pair<H, L>& destRegPair);

  void ExecJump(u16 targetLoc, bool shouldJump = true);
  void ExecCall(u16 targetLoc, bool shouldCall = true);
  template <u8 L>
  void ExecRestart();
  void ExecReturn(bool shouldReturn);
  void ExecReturn(); // uses one less IO access than ExecReturn(bool)
  void ExecOpJp0xe9();
  void ExecOpJr0x18();
  void ExecOpJr0x20();
  void ExecOpJr0x28();
  void ExecOpJr0x30();
  void ExecOpJr0x38();
  void ExecOpReti0xd9();

  void ExecAdd(u8 val);
  void ExecAddWithCarry(u8 val);
  void ExecSub(u8 val);
  void ExecSubWithCarry(u8 val);

  template <typename H, typename L>
  void ExecAdd16(CpuRegister8Pair<H, L>& destRegPair, u16 val);
  void ExecOpAdd0xe8();

  void ExecAnd(u8 val);
  void ExecOr(u8 val);
  void ExecXor(u8 val);
  void ExecCompare(u8 val);

  u8 ExecInc(u8 val);
  u8 ExecDec(u8 val);
  template <typename C>
  void ExecInc(CpuRegister<C, u8>& destReg);
  template <typename C>
  void ExecDec(CpuRegister<C, u8>& destReg);
  void ExecInc(u16 destLoc);
  void ExecDec(u16 destLoc);

  template <typename H, typename L>
  void ExecInc16(CpuRegister8Pair<H, L>& destRegPair);
  template <typename C>
  void ExecInc16(CpuRegister<C, u16>& destReg);
  template <typename H, typename L>
  void ExecDec16(CpuRegister8Pair<H, L>& destRegPair);
  template <typename C>
  void ExecDec16(CpuRegister<C, u16>& destReg);

  void ExecOpDaa0x27();
  void ExecOpCpl0x2f();
  void ExecOpScf0x37();
  void ExecOpCcf0x3f();
  void ExecOpCb0xcb();

  u8 ExecRotLeft(u8 val);
  u8 ExecRotLeftThroughCarry(u8 val);
  u8 ExecRotRight(u8 val);
  u8 ExecRotRightThroughCarry(u8 val);
  template <typename C>
  void ExecRotLeft(CpuRegister<C, u8>& destReg);
  template <typename C>
  void ExecRotLeftThroughCarry(CpuRegister<C, u8>& destReg);
  template <typename C>
  void ExecRotRight(CpuRegister<C, u8>& destReg);
  template <typename C>
  void ExecRotRightThroughCarry(CpuRegister<C, u8>& destReg);
  void ExecRotLeft(u16 destLoc);
  void ExecRotLeftThroughCarry(u16 destLoc);
  void ExecRotRight(u16 destLoc);
  void ExecRotRightThroughCarry(u16 destLoc);
  void ExecOpRlca0x07();
  void ExecOpRla0x17();
  void ExecOpRrca0x0f();
  void ExecOpRra0x1f();

  u8 ExecShiftLeft(u8 val);
  u8 ExecShiftRight(u8 val);
  u8 ExecShiftRightSigned(u8 val);
  template <typename C>
  void ExecShiftLeft(CpuRegister<C, u8>& destReg);
  template <typename C>
  void ExecShiftRight(CpuRegister<C, u8>& destReg);
  template <typename C>
  void ExecShiftRightSigned(CpuRegister<C, u8>& destReg);
  void ExecShiftLeft(u16 destLoc);
  void ExecShiftRight(u16 destLoc);
  void ExecShiftRightSigned(u16 destLoc);

  u8 ExecSwap(u8 val);
  template <typename C>
  void ExecSwap(CpuRegister<C, u8>& destReg);
  void ExecSwap(u16 destLoc);

  template <unsigned int B>
  void ExecTestBit(u8 val);
  template <unsigned int B>
  u8 ExecSetBit(u8 val);
  template <unsigned int B, typename C>
  void ExecSetBit(CpuRegister<C, u8>& destReg);
  template <unsigned int B>
  void ExecSetBit(u16 destLoc);
  template <unsigned int B>
  u8 ExecResetBit(u8 val);
  template <unsigned int B, typename C>
  void ExecResetBit(CpuRegister<C, u8>& destReg);
  template <unsigned int B>
  void ExecResetBit(u16 destLoc);
};

#include "hw/cpu/cpu_ops_inl.h"

#endif // SDGBC_CPU_H_
