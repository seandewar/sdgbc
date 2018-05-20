#include "hw/apu/apu.h"
#include "hw/cpu/cpu.h"
#include "emulator.h"
#include "util.h"
#include <cassert>
#include <limits>

// total amount of clock cycles needed for a frame sequencer update.
// frame seq updates at 512 Hz
constexpr auto kFrameSeqUpdateTotalCycles = kNormalSpeedClockRateHz / 512u;

// we need to downsample to a sample rate supported by modern systems.
// to do so, we only take samples every kCyclesPerBufferedSamples clock cycles
// TODO remove the minus constant when emulation speed is more accurate
constexpr auto kCyclesPerBufferedSamples = kNormalSpeedClockRateHz
                                           / kApuOutputSampleRateHz;

Apu::Apu(const Cpu& cpu)
    : cpu_(cpu), audioOut_(nullptr),
      muteCh1_(false), muteCh2_(false), muteCh3_(false), muteCh4_(false) {}

void Apu::Reset() {
  outSampleCycles_ = 0;
  frameSeqCycles_ = frameSeqStep_ = 0;

  // initial register values
  channelCtrl_ = 0x77;
  outCtrl_ = 0xf3;
  soundOn_ = true;

  ch1_.Reset();
  ch2_.Reset();
  ch3_.Reset();
  ch4_.Reset();
}

void Apu::Update(unsigned int cycles) {
  if (!soundOn_) {
    return;
  }

  // APU speed does not scale with double speed mode
  cycles = util::RescaleCycles(cpu_, cycles);

  for (auto i = 0u; i < cycles; ++i) {
    UpdateFrameSequencerCycle();

    ch1_.UpdateFrequencyTimer();
    ch2_.UpdateFrequencyTimer();
    ch3_.UpdateFrequencyTimer();
    ch4_.UpdateFrequencyTimer();

    // check if it's time to buffer more samples before continuing the update
    if (++outSampleCycles_ >= kCyclesPerBufferedSamples) {
      outSampleCycles_ -= kCyclesPerBufferedSamples;

      if (audioOut_ && !audioOut_->AudioIsMuted()) {
        const auto samples = MixChannels();
        audioOut_->AudioBufferSamples(samples.first, samples.second);
      }
    }
  }
}

void Apu::UpdateFrameSequencerCycle() {
  if (++frameSeqCycles_ < kFrameSeqUpdateTotalCycles) {
    return; // not time to update the frame seq yet
  }

  frameSeqCycles_ -= kFrameSeqUpdateTotalCycles;

  if (frameSeqStep_ % 2 == 0) {
    // clock 256 Hz length control (ch1-4)
    ch1_.UpdateLengthCounter();
    ch2_.UpdateLengthCounter();
    ch3_.UpdateLengthCounter();
    ch4_.UpdateLengthCounter();
  }

  if (frameSeqStep_ == 2 || frameSeqStep_ == 6) {
    // clock 128 Hz sweep (ch1)
    ch1_.UpdateSweep();
  } else if (frameSeqStep_ == 7) {
    // clock 64 Hz volume envelope (ch1,2,4)
    ch1_.UpdateEnvelope();
    ch2_.UpdateEnvelope();
    ch4_.UpdateEnvelope();
  }

  // we have to keep track of at most 8 steps
  if (++frameSeqStep_ >= 8) {
    frameSeqStep_ = 0;
  }
}

std::pair<i16, i16> Apu::MixChannels() const {
  i16 left = 0, right = 0;

  // mix the output volumes of the APU sound channels
  {
    u8 vol;

    // mix ch1
    vol = muteCh1_ ? 0 : ch1_.CalculateDacOutputVolume();
    left  += outCtrl_ & 0x10 ? vol : 0;
    right += outCtrl_ & 0x01 ? vol : 0;

    // mix ch2
    vol = muteCh2_ ? 0 : ch2_.CalculateDacOutputVolume();
    left  += outCtrl_ & 0x20 ? vol : 0;
    right += outCtrl_ & 0x02 ? vol : 0;

    // mix ch3
    vol = muteCh3_ ? 0 : ch3_.CalculateDacOutputVolume();
    left  += outCtrl_ & 0x40 ? vol : 0;
    right += outCtrl_ & 0x04 ? vol : 0;

    // mix ch4
    vol = muteCh4_ ? 0 : ch4_.CalculateDacOutputVolume();
    left  += outCtrl_ & 0x80 ? vol : 0;
    right += outCtrl_ & 0x08 ? vol : 0;
  }

  // scale output mixed volumes by output terminal volumes set in channel ctrl.
  // NOTE: due to the way the APU calculates this, a terminal volume of 0 still
  // produces sound (due to the +1 of the terminal volume value)
  const u8 leftVol = (channelCtrl_ >> 4) & 7;
  left  *= leftVol + 1;

  const u8 rightVol = channelCtrl_ & 7;
  right *= rightVol + 1;

  // scale mixed output volumes to the 16-bit signed sample range.
  // as there are 4 channels, which can at most have their mixed output volumes
  // multiplied by 8 (terminal volume of 7), we divide INT16_MAX by these values
  const auto sampleScale = std::numeric_limits<i16>::max()
                           / (kApuChannelMaxOutputVolume * 4 * 8);
  left  *= sampleScale;
  right *= sampleScale;

  return std::make_pair(left, right);
}

void Apu::WriteWaveRam8(u8 loc, u8 val) {
  ch3_.WriteWaveRam8(loc, val);
}

u8 Apu::ReadWaveRam8(u8 loc) const {
  return ch3_.ReadWaveRam8(loc);
}

u8 Apu::GetWaveRamLastWritten8() const {
  return ch3_.GetWaveRamLastWritten8();
}

void Apu::WriteCh1Register(ApuCh1Register reg, u8 val) {
  if (!soundOn_) {
    return;
  }

  switch (reg) {
    case ApuCh1Register::SweepCtrl:
      ch1_.SetSweepCtrl(val);
      break;
    case ApuCh1Register::LengthLoadDutyCtrl:
      ch1_.SetLengthLoadDutyCtrl(val);
      break;
    case ApuCh1Register::EnvelopeCtrl:
      ch1_.SetEnvelopeCtrl(val);
      break;
    case ApuCh1Register::FreqLoadLo:
      ch1_.SetFreqLoadLo(val);
      break;
    case ApuCh1Register::LengthCtrlFreqLoadHi:
      ch1_.SetLengthCtrlFreqLoadHi(val);
      break;
    default: assert(!"unknown apu ch1 register write!");
  }
}

u8 Apu::ReadCh1Register(ApuCh1Register reg) const {
  switch (reg) {
    case ApuCh1Register::SweepCtrl:
      return ch1_.GetSweepCtrl();
    case ApuCh1Register::LengthLoadDutyCtrl:
      return ch1_.GetLengthLoadDutyCtrl();
    case ApuCh1Register::EnvelopeCtrl:
      return ch1_.GetEnvelopeCtrl();
    case ApuCh1Register::LengthCtrlFreqLoadHi:
      return ch1_.GetLengthCtrl();

    default: assert(!"unknown apu ch1 register read!");
    case ApuCh1Register::FreqLoadLo:
      return 0xff;
  }
}

void Apu::WriteCh2Register(ApuCh2Register reg, u8 val) {
  if (!soundOn_) {
    return;
  }

  switch (reg) {
    case ApuCh2Register::LengthLoadDutyCtrl:
      ch2_.SetLengthLoadDutyCtrl(val);
      break;
    case ApuCh2Register::EnvelopeCtrl:
      ch2_.SetEnvelopeCtrl(val);
      break;
    case ApuCh2Register::FreqLoadLo:
      ch2_.SetFreqLoadLo(val);
      break;
    case ApuCh2Register::LengthCtrlFreqLoadHi:
      ch2_.SetLengthCtrlFreqLoadHi(val);
      break;
    default: assert(!"unknown apu ch2 register write!");
  }
}

u8 Apu::ReadCh2Register(ApuCh2Register reg) const {
  switch (reg) {
    case ApuCh2Register::LengthLoadDutyCtrl:
      return ch2_.GetLengthLoadDutyCtrl();
    case ApuCh2Register::EnvelopeCtrl:
      return ch2_.GetEnvelopeCtrl();
    case ApuCh2Register::LengthCtrlFreqLoadHi:
      return ch2_.GetLengthCtrl();

    default: assert(!"unknown apu ch2 register read!");
    case ApuCh2Register::FreqLoadLo:
      return 0xff;
  }
}

void Apu::WriteCh3Register(ApuCh3Register reg, u8 val) {
  if (!soundOn_) {
    return;
  }

  switch (reg) {
    case ApuCh3Register::DacOnCtrl:
      ch3_.SetDacOnCtrl(val);
      break;
    case ApuCh3Register::LengthLoad:
      ch3_.SetLengthLoad(val);
      break;
    case ApuCh3Register::VolumeCtrl:
      ch3_.SetVolumeCtrl(val);
      break;
    case ApuCh3Register::FreqLoadLo:
      ch3_.SetFreqLoadLo(val);
      break;
    case ApuCh3Register::LengthCtrlFreqLoadHi:
      ch3_.SetLengthCtrlFreqLoadHi(val);
      break;
    default: assert(!"unknown apu ch3 register write!");
  }
}

u8 Apu::ReadCh3Register(ApuCh3Register reg) const {
  switch (reg) {
    case ApuCh3Register::DacOnCtrl:
      return ch3_.GetDacOnCtrl();
    case ApuCh3Register::VolumeCtrl:
      return ch3_.GetVolumeCtrl();
    case ApuCh3Register::LengthCtrlFreqLoadHi:
      return ch3_.GetLengthCtrl();

    default: assert(!"unknown apu ch3 register read!");
    case ApuCh3Register::FreqLoadLo:
    case ApuCh3Register::LengthLoad:
      return 0xff;
  }
}

void Apu::WriteCh4Register(ApuCh4Register reg, u8 val) {
  if (!soundOn_) {
    return;
  }

  switch (reg) {
    case ApuCh4Register::LengthLoad:
      ch4_.SetLengthLoad(val);
      break;
    case ApuCh4Register::EnvelopeCtrl:
      ch4_.SetEnvelopeCtrl(val);
      break;
    case ApuCh4Register::PolynomialCtrl:
      ch4_.SetPolynomialCtrl(val);
      break;
    case ApuCh4Register::LengthCtrl:
      ch4_.SetLengthCtrl(val);
      break;
    default: assert(!"unknown apu ch4 register write!");
  }
}

u8 Apu::ReadCh4Register(ApuCh4Register reg) const {
  switch (reg) {
    case ApuCh4Register::EnvelopeCtrl:
      return ch4_.GetEnvelopeCtrl();
    case ApuCh4Register::PolynomialCtrl:
      return ch4_.GetPolynomialCtrl();
    case ApuCh4Register::LengthCtrl:
      return ch4_.GetLengthCtrl();

    default: assert(!"unknown apu ch4 register read!");
    case ApuCh4Register::LengthLoad:
      return 0xff;
  }
}

void Apu::ZeroWriteAllRegisters() {
  channelCtrl_ = outCtrl_ = 0;

  WriteCh1Register(ApuCh1Register::SweepCtrl, 0);
  WriteCh1Register(ApuCh1Register::LengthLoadDutyCtrl, 0);
  WriteCh1Register(ApuCh1Register::EnvelopeCtrl, 0);
  WriteCh1Register(ApuCh1Register::FreqLoadLo, 0);
  WriteCh1Register(ApuCh1Register::LengthCtrlFreqLoadHi, 0);

  WriteCh2Register(ApuCh2Register::LengthLoadDutyCtrl, 0);
  WriteCh2Register(ApuCh2Register::EnvelopeCtrl, 0);
  WriteCh2Register(ApuCh2Register::FreqLoadLo, 0);
  WriteCh2Register(ApuCh2Register::LengthCtrlFreqLoadHi, 0);

  WriteCh3Register(ApuCh3Register::DacOnCtrl, 0);
  WriteCh3Register(ApuCh3Register::LengthLoad, 0);
  WriteCh3Register(ApuCh3Register::VolumeCtrl, 0);
  WriteCh3Register(ApuCh3Register::FreqLoadLo, 0);
  WriteCh3Register(ApuCh3Register::LengthCtrlFreqLoadHi, 0);

  WriteCh4Register(ApuCh4Register::LengthLoad, 0);
  WriteCh4Register(ApuCh4Register::EnvelopeCtrl, 0);
  WriteCh4Register(ApuCh4Register::PolynomialCtrl, 0);
  WriteCh4Register(ApuCh4Register::LengthCtrl, 0);
}

void Apu::SetChannelCtrl(u8 val) {
  if (soundOn_) {
    channelCtrl_ = val;
  }
}

u8 Apu::GetChannelCtrl() const {
  return channelCtrl_;
}

void Apu::SetOutputCtrl(u8 val) {
  if (soundOn_) {
    outCtrl_ = val;
  }
}

u8 Apu::GetOutputCtrl() const {
  return outCtrl_;
}

void Apu::SetOnCtrl(u8 val) {
  const bool newOn = (val & 0x80) != 0;

  if (newOn != soundOn_) {
    if (!newOn) {
      ZeroWriteAllRegisters();
    }
    else {
      // reset frame seq step, square duty and clear wave RAM
      frameSeqStep_ = 0;

      ch1_.ResetDutyCounter();
      ch2_.ResetDutyCounter();
      ch3_.ClearWaveRam();
    }
  }

  soundOn_ = newOn;
}

u8 Apu::GetOnCtrl() const {
  u8 onCtrl = soundOn_ ? 0xf0 : 0x70;
  onCtrl |= (ch1_.IsEnabled() ? 1 : 0);
  onCtrl |= (ch2_.IsEnabled() ? 2 : 0);
  onCtrl |= (ch3_.IsEnabled() ? 4 : 0);
  onCtrl |= (ch4_.IsEnabled() ? 8 : 0);

  return onCtrl;
}

void Apu::SetMuteCh1(bool val) {
  muteCh1_ = val;
}

bool Apu::IsCh1Muted() const {
  return muteCh1_;
}

void Apu::SetMuteCh2(bool val) {
  muteCh2_ = val;
}

bool Apu::IsCh2Muted() const {
  return muteCh2_;
}

void Apu::SetMuteCh3(bool val) {
  muteCh3_ = val;
}

bool Apu::IsCh3Muted() const {
  return muteCh3_;
}

void Apu::SetMuteCh4(bool val) {
  muteCh4_ = val;
}

bool Apu::IsCh4Muted() const {
  return muteCh4_;
}

void Apu::SetApuOutput(IApuOutput* audioOut) {
  audioOut_ = audioOut;
}
