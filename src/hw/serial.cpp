#include "hw/cpu/cpu.h"
#include "hw/serial.h"

// the amount of clock cycles needed for a single bit transfer.
// normal/fast transfer speed modes are controlled by SC bit 1 (CGB mode only)
// (not to be confused with CPU normal and double-speed modes, but these modes
// do also affect the speed of the transfer)
constexpr auto kBitTransferNormalCycles = 512u,
               kBitTransferFastCycles   = 16u;

Serial::Serial(Cpu& cpu) : cpu_(cpu), dataOut_(nullptr) {}

void Serial::Reset(bool cgbMode) {
  cgbMode_ = cgbMode;

  nextBitTransferCycles_ = transferNextBitIdx_ = 0;

  sb_ = 0x00;
  sc_ = 0x7e;

  if (dataOut_) {
    dataOut_->SerialReset();
  }
}

void Serial::Update(unsigned int cycles) {
  // SC bit 7 is unset if there is no transfer in progress.
  // NOTE: because we're not bothering to implement serial properly, we'll do
  // nothing if we're set to listen for incoming data (SC bit 0 unset)
  if (!(sc_ & 0x80) || !(sc_ & 1)) {
    return;
  }

  nextBitTransferCycles_ += cycles;

  // fast transfer mode is CGB mode and bit 1 of SC set
  const auto transferCycles = cgbMode_ && sc_ & 2 ? kBitTransferFastCycles
                                                  : kBitTransferNormalCycles;

  while (nextBitTransferCycles_ > transferCycles) {
    nextBitTransferCycles_ -= transferCycles;

    // send SB bit 7 and shift it out of the register by shifting left.
    // replace the new bit 0 with the received bit.
    // NOTE: because we're ignoring serial for the project, we'll receive a 1.
    // this is usually what's received when there is no connection
    if (dataOut_) {
      dataOut_->SerialWriteBit(((sb_ >> 7) & 1) != 0);
    }
    sb_ = ((sb_ << 1) | 1) & 0xff;

    transferNextBitIdx_ = (transferNextBitIdx_ + 1) % 8;
    if (transferNextBitIdx_ == 0) {
      // finished sending a full byte. request int $58 and clear SC bit 7
      cpu_.IntfRequest(kCpuInterrupt0x58);
      sc_ &= 0x7f;

      if (dataOut_) {
        dataOut_->SerialOnByteWritten();
      }
    }
  }
}

void Serial::SetSerialOutput(ISerialOutput* dataOut) {
  dataOut_ = dataOut;
}

void Serial::SetSb(u8 val) {
  sb_ = val;
}

u8 Serial::GetSb() const {
  return sb_;
}

void Serial::SetSc(u8 val) {
  sc_ = val | 0x7c; // bits 2-6 unused
}

u8 Serial::GetSc() const {
  return sc_;
}

bool Serial::IsInCgbMode() const {
  return cgbMode_;
}