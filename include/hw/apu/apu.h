#ifndef SDGBC_APU_H_
#define SDGBC_APU_H_

#include "hw/apu/apu_chan_noise.h"
#include "hw/apu/apu_chan_square.h"
#include "hw/apu/apu_chan_wave.h"
#include "types.h"
#include <utility>

enum class ApuCh1Register {
  SweepCtrl,
  LengthLoadDutyCtrl,
  EnvelopeCtrl,
  FreqLoadLo,
  LengthCtrlFreqLoadHi
};

enum class ApuCh2Register {
  LengthLoadDutyCtrl,
  EnvelopeCtrl,
  FreqLoadLo,
  LengthCtrlFreqLoadHi
};

enum class ApuCh3Register {
  DacOnCtrl,
  LengthLoad,
  VolumeCtrl,
  FreqLoadLo,
  LengthCtrlFreqLoadHi
};

enum class ApuCh4Register {
  LengthLoad,
  EnvelopeCtrl,
  PolynomialCtrl,
  LengthCtrl
};

// CD-quality sound output rate
constexpr auto kApuOutputSampleRateHz = 44100u;

class IApuOutput {
public:
  virtual ~IApuOutput() = default;

  virtual void AudioBufferSamples(i16 leftSample, i16 rightSample) = 0;
  virtual bool AudioIsMuted() const = 0;
};

class Cpu;

class Apu {
public:
  explicit Apu(const Cpu& cpu);

  void Reset();

  void Update(unsigned int cycles);

  void SetApuOutput(IApuOutput* audioOut);

  void WriteWaveRam8(u8 loc, u8 val);
  u8 ReadWaveRam8(u8 loc) const;
  u8 GetWaveRamLastWritten8() const;

  void WriteCh1Register(ApuCh1Register reg, u8 val);
  u8 ReadCh1Register(ApuCh1Register reg) const;

  void WriteCh2Register(ApuCh2Register reg, u8 val);
  u8 ReadCh2Register(ApuCh2Register reg) const;

  void WriteCh3Register(ApuCh3Register reg, u8 val);
  u8 ReadCh3Register(ApuCh3Register reg) const;

  void WriteCh4Register(ApuCh4Register reg, u8 val);
  u8 ReadCh4Register(ApuCh4Register reg) const;

  void SetChannelCtrl(u8 val);
  u8 GetChannelCtrl() const;

  void SetOutputCtrl(u8 val);
  u8 GetOutputCtrl() const;

  void SetOnCtrl(u8 val);
  u8 GetOnCtrl() const;

  void SetMuteCh1(bool val);
  bool IsCh1Muted() const;

  void SetMuteCh2(bool val);
  bool IsCh2Muted() const;

  void SetMuteCh3(bool val);
  bool IsCh3Muted() const;

  void SetMuteCh4(bool val);
  bool IsCh4Muted() const;

private:
  const Cpu& cpu_;

  IApuOutput* audioOut_;

  ApuSquareSweepChannel ch1_;
  ApuSquareChannel ch2_;
  ApuWaveChannel ch3_;
  ApuNoiseChannel ch4_;

  bool muteCh1_, muteCh2_, muteCh3_, muteCh4_;

  unsigned int frameSeqCycles_, frameSeqStep_;
  unsigned int outSampleCycles_;

  // sound control registers
  u8 channelCtrl_, outCtrl_;
  bool soundOn_;

  void UpdateFrameSequencerCycle();

  void ZeroWriteAllRegisters();

  // returns the samples for both the left and right channels
  std::pair<i16, i16> MixChannels() const;
};

#endif // SDGBC_APU_H_
