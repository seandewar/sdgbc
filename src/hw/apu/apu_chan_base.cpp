#include "hw/apu/apu_chan_base.h"
#include <cassert>

// if (envelopeCtrl & mask) is not 0, DAC is enabled.
// NOTE: the wave channel does not have an envelope ctrl register. DAC is
// instead turned on/off via the channel's on ctrl register (NR30)
constexpr u8 kEnvelopeCtrlDacEnabledMask = 0xf8;

void ApuSoundChannelBase::Reset() {
  enabled_ = dacEnabled_ = false;

  lengthEnabled_ = false;
  freqTimer_ = lengthCounter_ = 0;
}

void ApuSoundChannelBase::Restart() {
  enabled_ = dacEnabled_;

  if (lengthCounter_ <= 0) {
    ResetLengthCounter();
  }

  freqTimer_ = GetFrequencyTimerPeriod();
}

void ApuSoundChannelBase::UpdateFrequencyTimer() {
  // NOTE: if the timer is already 0, it will overflow and not trigger this
  // condition - audio seems to sound better (especially with the noise channel)
  // than when handling that case!
  if (--freqTimer_ <= 0) {
    freqTimer_ = GetFrequencyTimerPeriod();
    UpdateFrequency();
  }
}

void ApuSoundChannelBase::UpdateLengthCounter() {
  if (lengthEnabled_ && lengthCounter_ > 0 && --lengthCounter_ <= 0) {
    enabled_ = false;
  }
}

u8 ApuSoundChannelBase::CalculateDacOutputVolume() const {
  const u8 vol = (enabled_ && IsDacEnabled() ? CalculateOutputVolume() : 0);
  assert(vol <= kApuChannelMaxOutputVolume);

  return vol;
}

void ApuSoundChannelBase::SetLengthCtrl(u8 val) {
  lengthEnabled_ = (val & 0x40) != 0;

  // restart if bit 7 is set
  if (val & 0x80) {
    Restart();
  }
}

u8 ApuSoundChannelBase::GetLengthCtrl() const {
  return lengthEnabled_ ? 0xff : 0xbf; // only bit 6 readable
}

bool ApuSoundChannelBase::IsEnabled() const {
  return enabled_;
}

void ApuSoundChannelBase::ResetLengthCounter(u8 lengthSubtract) {
  lengthCounter_ = GetMaxLength() - lengthSubtract;
}

void ApuSoundChannelBase::SetDacEnabled(bool val) {
  dacEnabled_ = val;

  // NOTE: enabling DAC again won't re-enable the channel
  if (!dacEnabled_) {
    enabled_ = false;
  }
}

bool ApuSoundChannelBase::IsDacEnabled() const {
  return dacEnabled_;
}

void ApuEnvelopeChannelBase::Reset() {
  ApuSoundChannelBase::Reset();

  envTimer_ = envVolume_ = 0;
  envCtrl_ = 0x00;
}

void ApuEnvelopeChannelBase::Restart() {
  ApuSoundChannelBase::Restart();

  ResetEnvelopeTimer();
  ResetEnvelopeVolume();
}

void ApuEnvelopeChannelBase::UpdateEnvelope() {
  if (--envTimer_ <= 0 && ResetEnvelopeTimer() > 0) {
    // update current envelope volume depending on the add mode (bit 3).
    // the new volume cannot be greater than 15 or below 0
    if (envCtrl_ & 8) {
      if (envVolume_ < 15) {
        ++envVolume_;
      }
    } else {
      if (envVolume_ > 0) {
        --envVolume_;
      }
    }
  }
}

u8 ApuEnvelopeChannelBase::ResetEnvelopeTimer() {
  // reload the period from bits 7-4 of the ctrl reg.
  // due to weird APU behaviour, a new period of 0 is treated as 8 instead
  const u8 newPeriod = envCtrl_ & 7;
  envTimer_ = newPeriod == 0 ? 8 : newPeriod;
  return newPeriod;
}

void ApuEnvelopeChannelBase::SetEnvelopeCtrl(u8 val) {
  envCtrl_ = val;
  SetDacEnabled((envCtrl_ & kEnvelopeCtrlDacEnabledMask) != 0);
}

u8 ApuEnvelopeChannelBase::GetEnvelopeCtrl() const {
  return envCtrl_;
}

void ApuEnvelopeChannelBase::ResetEnvelopeVolume() {
  envVolume_ = (envCtrl_ & 0xf0) >> 4;
}

u16 ApuEnvelopeChannelBase::GetMaxLength() const {
  return 64;
}
