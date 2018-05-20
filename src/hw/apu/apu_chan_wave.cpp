#include "hw/apu/apu_chan_wave.h"
#include "util.h"
#include <cassert>

void ApuWaveChannel::Reset() {
  ApuSoundChannelBase::Reset();

  freqLoad_ = 0x0000;
  volumeCode_ = 0;
  sampleIdxCounter_ = 0;

  // on the CGB, wave RAM has consistent values, repeating $00 and $FF
  for (std::size_t i = 0; i < waveRam_.size(); ++i) {
    waveRam_[i] = (i % 2 == 0 ? 0x00 : 0xff);
  }
  waveRamLastWrittenVal_ = 0xff;
}

void ApuWaveChannel::Restart() {
  ApuSoundChannelBase::Restart();
  sampleIdxCounter_ = 0;
}

void ApuWaveChannel::UpdateFrequency() {
  // increment the sample index counter.
  // sample is 4 bits, so wave RAM has (2 * (size of wave RAM bytes)) samples
  sampleIdxCounter_ = (sampleIdxCounter_ + 1) % (waveRam_.size() * 2);
}

u8 ApuWaveChannel::CalculateOutputVolume() const {
  // first 4-bit sample is stored in the high nibble
  const u8 twoSamples = waveRam_[sampleIdxCounter_ / 2];
  const u8 sample = sampleIdxCounter_ % 2 == 0 ? (twoSamples >> 4) & 0xf
                                               : twoSamples & 0xf;

  // calculate the output vol by shifting right using the volume code.
  // a code of 0 produces silence, 1-3 indicates a right shift of (code - 1)
  assert(volumeCode_ <= 3);
  return volumeCode_ > 0 ? sample >> (volumeCode_ - 1) : 0;
}

void ApuWaveChannel::ClearWaveRam() {
  waveRam_.fill(waveRamLastWrittenVal_ = 0x00);
}

void ApuWaveChannel::WriteWaveRam8(u8 loc, u8 val) {
  assert(loc < waveRam_.size());
  waveRam_[loc] = waveRamLastWrittenVal_ = val;
}

u8 ApuWaveChannel::ReadWaveRam8(u8 loc) const {
  assert(loc < waveRam_.size());
  return waveRam_[loc];
}

u8 ApuWaveChannel::GetWaveRamLastWritten8() const {
  return waveRamLastWrittenVal_;
}

void ApuWaveChannel::SetDacOnCtrl(u8 val) {
  SetDacEnabled((val & 0x80) != 0);
}

u8 ApuWaveChannel::GetDacOnCtrl() const {
  return IsDacEnabled() ? 0xff : 0x7f;
}

void ApuWaveChannel::SetLengthLoad(u8 val) {
  ResetLengthCounter(val);
}

void ApuWaveChannel::SetVolumeCtrl(u8 val) {
  volumeCode_ = (val >> 5) & 3; // code number in bits 5-6
}

u8 ApuWaveChannel::GetVolumeCtrl() const {
  return (volumeCode_ << 5) | 0x9f;
}

void ApuWaveChannel::SetFreqLoadLo(u8 val) {
  freqLoad_ = util::SetLo8(freqLoad_, val);
}

void ApuWaveChannel::SetLengthCtrlFreqLoadHi(u8 val) {
  freqLoad_ = util::SetHi8(freqLoad_, val & 7); // bits 0-2 freq MSB
  SetLengthCtrl(val);
}

u16 ApuWaveChannel::GetFrequencyTimerPeriod() const {
  return ((kApuChannelMaxFreqLoad + 1) - freqLoad_) * 2;
}

u16 ApuWaveChannel::GetMaxLength() const {
  return 256;
}
