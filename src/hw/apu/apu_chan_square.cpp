#include "hw/apu/apu_chan_square.h"
#include "util.h"

constexpr std::array<u8, 4> kSquareDutyWaveforms {
  0b00000001,
  0b10000001,
  0b10000111,
  0b01111110
};

void ApuSquareChannel::Reset() {
  ApuEnvelopeChannelBase::Reset();

  freqLoad_ = 0x0000;
  dutyNum_ = dutyBitIdxCounter_ = 0;
}

void ApuSquareChannel::ResetDutyCounter() {
  dutyBitIdxCounter_ = 0;
}

void ApuSquareChannel::UpdateFrequency() {
  // increment the duty bit pos
  dutyBitIdxCounter_ = (dutyBitIdxCounter_ + 1) % 8;
}

u8 ApuSquareChannel::CalculateOutputVolume() const {
  // output envelope volume if the selected bit of the duty waveform is set
  const u8 duty = kSquareDutyWaveforms[dutyNum_];
  return (1 << dutyBitIdxCounter_) & duty ? envVolume_ : 0;
}

u16 ApuSquareChannel::GetFrequencyTimerPeriod() const {
  return ((kApuChannelMaxFreqLoad + 1) - freqLoad_) * 4;
}

void ApuSquareChannel::SetLengthLoadDutyCtrl(u8 val) {
  dutyNum_ = (val >> 6) & 3; // duty number is bits 6-7
  ResetLengthCounter(val & 0x3f); // load value is bits 0-5
}

u8 ApuSquareChannel::GetLengthLoadDutyCtrl() const {
  return (dutyNum_ << 6) | 0x3f; // only duty number readable (bits 6-7)
}

void ApuSquareChannel::SetFreqLoadLo(u8 val) {
  freqLoad_ = util::SetLo8(freqLoad_, val);
}

void ApuSquareChannel::SetLengthCtrlFreqLoadHi(u8 val) {
  freqLoad_ = util::SetHi8(freqLoad_, val & 7); // bits 0-2 freq MSB
  SetLengthCtrl(val);
}

void ApuSquareSweepChannel::Reset() {
  ApuSquareChannel::Reset();

  // sweep channel starts enabled (with DAC on), with duty number 2
  enabled_ = true;
  envCtrl_ = 0xf3;
  dutyNum_ = 2;

  sweepCtrl_ = 0x80;
  sweepEnabled_ = false;
  ResetSweepShadowFrequency();
}

void ApuSquareSweepChannel::Restart() {
  ApuSquareChannel::Restart();

  ResetSweepShadowFrequency();
  sweepEnabled_ = ResetSweepTimer() > 0 || GetSweepShift() > 0;

  if (GetSweepShift() > 0) {
    // perform a new frequency calc without the write to check for overflow
    CalculateNewSweepFreq();
  }
}

void ApuSquareSweepChannel::UpdateSweep() {
  if (--sweepTimer_ <= 0 && ResetSweepTimer() > 0 && sweepEnabled_) {
    const u16 newFreq = CalculateNewSweepFreq();

    if (newFreq <= kApuChannelMaxFreqLoad && GetSweepShift() > 0) {
      // the freq ctrl initial freq value is changed to the new freq, and the
      // new freq replaces shadow freq, and a new freq calc is performed (its
      // value is NOT written and is ignored, but it modifies internal state)
      freqLoad_ = sweepShadow_ = newFreq;
      CalculateNewSweepFreq();
    }
  }
}

u16 ApuSquareSweepChannel::CalculateNewSweepFreq() {
  // shift shadow freq right using the sweep shift value. this value is either
  // subtracted or added to the shadow freq depending on ctrl bit 3
  const u16 newFreq = sweepShadow_ + ((sweepCtrl_ & 8 ? -1 : 1)
                                      * (sweepShadow_ >> GetSweepShift()));
  if (newFreq > kApuChannelMaxFreqLoad) {
    // internal overflow check - disables channel
    enabled_ = false;
  }

  return newFreq;
}

u8 ApuSquareSweepChannel::ResetSweepTimer() {
  // reload the period from bits 7-4 of the ctrl reg.
  // due to weird APU behaviour, a new period of 0 is treated as 8 instead
  const u8 newPeriod = (sweepCtrl_ >> 4) & 7;
  sweepTimer_ = newPeriod == 0 ? 8 : newPeriod;
  return newPeriod;
}

void ApuSquareSweepChannel::ResetSweepShadowFrequency() {
  sweepShadow_ = freqLoad_;
}

u8 ApuSquareSweepChannel::GetSweepShift() const {
  return sweepCtrl_ & 7;
}

void ApuSquareSweepChannel::SetSweepCtrl(u8 val) {
  sweepCtrl_ = val;
}

u8 ApuSquareSweepChannel::GetSweepCtrl() const {
  return sweepCtrl_;
}
