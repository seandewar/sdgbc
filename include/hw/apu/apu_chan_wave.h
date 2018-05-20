#ifndef SDGBC_APU_CHAN_WAVE_H_
#define SDGBC_APU_CHAN_WAVE_H_

#include "hw/apu/apu_chan_base.h"

class ApuWaveChannel : public ApuSoundChannelBase {
public:
  void Reset() override;
  void Restart() override;

  void ClearWaveRam();

  void WriteWaveRam8(u8 loc, u8 val);
  u8 ReadWaveRam8(u8 loc) const;
  u8 GetWaveRamLastWritten8() const;

  void SetDacOnCtrl(u8 val);
  u8 GetDacOnCtrl() const;

  void SetLengthLoad(u8 val);

  void SetVolumeCtrl(u8 val);
  u8 GetVolumeCtrl() const;

  void SetFreqLoadLo(u8 val);
  void SetLengthCtrlFreqLoadHi(u8 val);

private:
  u16 freqLoad_;
  u8 volumeCode_;

  WaveRam waveRam_;
  u8 waveRamLastWrittenVal_;
  u8 sampleIdxCounter_;

  void UpdateFrequency() override;
  u8 CalculateOutputVolume() const override;

  u16 GetFrequencyTimerPeriod() const override;
  u16 GetMaxLength() const override;
};

#endif // SDGBC_APU_CHAN_WAVE_H_
