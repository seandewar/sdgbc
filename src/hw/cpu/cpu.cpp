#include "hw/cpu/cpu.h"
#include "hw/dma.h"
#include "hw/joypad.h"
#include "hw/mmu.h"
#include <cassert>

Cpu::Cpu(Mmu& mmu, const Dma& dma, const Joypad& joypad)
    : status_(CpuStatus::Hung), mmu_(mmu), dma_(dma), joypad_(joypad) {}

void Cpu::Reset(bool cgbMode) {
  cgbMode_ = cgbMode;

  // set initial register values; this is usually done by the boot ROM.
  // in DMG mode, set the accumulator to the value set by the DMG boot ROM.
  // this allows games to auto-detect DMG mode when we force it on
  reg_.pc.Set(0x0100);
  reg_.sp.Set(0xfffe);
  reg_.af.Set(cgbMode_ ? 0x1180 : 0x0180);
  reg_.bc.Set(0x0000);
  reg_.de.Set(0x0008);
  reg_.hl.Set(0x007c);

  // disable interrupts and reset int flags to reset vals
  intme_ = false;
  intf_ = 0xe1;
  inte_ = 0x00;

  // reset cpu state and ready it for running
  status_ = CpuStatus::Running;
  speedSwitchRequested_ = doubleSpeedMode_ = false;
  speedSwitchCyclesLeft_ = 0;
}

bool Cpu::Resume() {
  if (status_ != CpuStatus::Hung && speedSwitchCyclesLeft_ <= 0) {
    status_ = CpuStatus::Running;
    return true;
  }

  return false;
}

unsigned int Cpu::Update() {
  updateCycles_ = 0;
  InternalDelay(); // spin for 4 clock cycles by default

  if (status_ != CpuStatus::Hung && !dma_.IsNdmaInProgress()) {
    if (status_ == CpuStatus::Stopped) {
      HandleStoppedUpdate();
    } else if (!HandleInterrupts() &&
               status_ == CpuStatus::Running &&
               !ExecuteOp(mmu_.Read8(reg_.pc++))) {
      // unknown opcode executed - hang
      status_ = CpuStatus::Hung;
    }
  }

  return updateCycles_;
}

void Cpu::HandleStoppedUpdate() {
  if (joypad_.WasSelectedKeyPressed()) {
    // a selected key press during the speed switch hangs the CPU,
    // otherwise, wake up
    status_ = IsSpeedSwitchInProgress() ? CpuStatus::Hung
                                        : CpuStatus::Running;
  } else if (IsSpeedSwitchInProgress()) {
    if (updateCycles_ >= speedSwitchCyclesLeft_) {
      // double speed switch finished, wake up
      speedSwitchRequested_ = false;
      speedSwitchCyclesLeft_ = 0;

      doubleSpeedMode_ = !doubleSpeedMode_;
      status_ = CpuStatus::Running;
    } else {
      speedSwitchCyclesLeft_ -= updateCycles_;
    }
  }
}

bool Cpu::IsSpeedSwitchInProgress() const {
  return speedSwitchCyclesLeft_ > 0;
}

bool Cpu::HandleInterrupts() {
  // have an int to service if both the enable and flag bits for an int are set
  // and the master interrupt enable is on
  if (intf_ & inte_) {
    if (intme_) {
      // service one int prioritised from vec $40 to $60, then clear the flag bit
      if (inte_ & intf_ & kCpuInterrupt0x40) {
        intf_ &= ~kCpuInterrupt0x40;
        ServiceInterrupt<0x40>();
      } else if (inte_ & intf_ & kCpuInterrupt0x48) {
        intf_ &= ~kCpuInterrupt0x48;
        ServiceInterrupt<0x48>();
      } else if (inte_ & intf_ & kCpuInterrupt0x50) {
        intf_ &= ~kCpuInterrupt0x50;
        ServiceInterrupt<0x50>();
      } else if (inte_ & intf_ & kCpuInterrupt0x58) {
        intf_ &= ~kCpuInterrupt0x58;
        ServiceInterrupt<0x58>();
      } else if (inte_ & intf_ & kCpuInterrupt0x60) {
        intf_ &= ~kCpuInterrupt0x60;
        ServiceInterrupt<0x60>();
      }
    }

    // resume from suspension, even if a requested interrupt couldn't be
    // serviced due to intme being off
    status_ = CpuStatus::Running;
    return intme_;
  }

  return false;
}

void Cpu::InternalDelay(unsigned int numAccesses) {
  updateCycles_ += 4 * numAccesses; // each access takes 4 clock cycles
}

u8 Cpu::IoRead8(u16 loc) {
  InternalDelay();
  return mmu_.Read8(loc);
}

void Cpu::IoWrite8(u16 loc, u8 val) {
  InternalDelay();
  mmu_.Write8(loc, val);
}

u8 Cpu::IoPcReadNext8() {
  return IoRead8(reg_.pc++);
}

u16 Cpu::IoPcReadNext16() {
  const u8 lo = IoPcReadNext8();
  const u8 hi = IoPcReadNext8();
  return util::To16(hi, lo);
}

const CpuRegisters& Cpu::GetRegisters() const {
  return reg_;
}

CpuStatus Cpu::GetStatus() const {
  return status_;
}

bool Cpu::GetIntme() const {
  return intme_;
}

void Cpu::SetInte(u8 val) {
  inte_ = val & 0x1f;
}

u8 Cpu::GetInte() const {
  return inte_;
}

void Cpu::SetIntf(u8 val) {
  intf_ = val & 0x1f;
}

void Cpu::IntfRequest(u8 mask) {
  intf_ |= mask & 0x1f;
}

u8 Cpu::GetIntf() const {
  return intf_;
}

void Cpu::SetKey1(u8 val) {
  if (cgbMode_) {
    speedSwitchRequested_ = (val & 1) != 0;
  }
}

u8 Cpu::GetKey1() const {
  if (cgbMode_) {
    return 0x7e | (doubleSpeedMode_ ? 0x80 : 0)
                | (speedSwitchRequested_ ? 1 : 0);
  } else {
    return 0xff;
  }
}

bool Cpu::IsInCgbMode() const {
  return cgbMode_;
}

bool Cpu::IsInDoubleSpeedMode() const {
  return doubleSpeedMode_;
}
