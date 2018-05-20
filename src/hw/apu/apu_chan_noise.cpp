#include "hw/apu/apu_chan_noise.h"

constexpr std::array<u8, 8> kNoiseFreqDivisors {
  8, 16, 32, 48, 64, 80, 96, 112
};

void ApuNoiseChannel::Reset() {
  ApuEnvelopeChannelBase::Reset();

  polyCtrl_ = 0x00;
  linearShift_ = 0;
}

void ApuNoiseChannel::Restart() {
  ApuEnvelopeChannelBase::Restart();
  linearShift_ = 0x7fff;
}

void ApuNoiseChannel::UpdateFrequency() {
  // generate a 15-bit pseudo-random bit sequence using the linear feedback
  // shift register (LSFR):
  //
  // XOR bits 0 & 1 of the LFSR, then right shift.
  // replace the now unset bit 7 with the XOR result bit
  const u8 xorBit = (linearShift_ & 1) ^ ((linearShift_ >> 1) & 1);
  linearShift_ = (linearShift_ >> 1) | (xorBit << 14);

  // if width mode (poly ctrl bit 3) set, also replace bit 6 with the XOR bit
  if (polyCtrl_ & 8) {
    linearShift_ = (linearShift_ & 0xbf) | (xorBit << 6);
  }
}

u8 ApuNoiseChannel::CalculateOutputVolume() const {
  // output the envelope volume if bit 0 of the LFSR is unset
  return linearShift_ & 1 ? 0 : envVolume_;
}

u16 ApuNoiseChannel::GetFrequencyTimerPeriod() const {
  // extract the noise frequency divisor number and freq clock shift amount
  // from the polynomial ctrl
  const u8 divisorNum = polyCtrl_ & 0x7; // bits 0-2
  const u8 freqShift = (polyCtrl_ >> 4) & 0xf; // bits 4-7

  return kNoiseFreqDivisors[divisorNum] << freqShift;
}

void ApuNoiseChannel::SetLengthLoad(u8 val) {
  ResetLengthCounter(val & 0x3f); // bits 0-5 writable
}

void ApuNoiseChannel::SetPolynomialCtrl(u8 val) {
  polyCtrl_ = val;
}

u8 ApuNoiseChannel::GetPolynomialCtrl() const {
  return polyCtrl_;
}
