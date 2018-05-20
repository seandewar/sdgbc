#ifndef SDGBC_APU_CHAN_BASE_H_
#define SDGBC_APU_CHAN_BASE_H_

#include "hw/memory.h"
#include "types.h"

constexpr u8 kApuChannelMaxOutputVolume = 15;
constexpr u16 kApuChannelMaxFreqLoad = 0x07ff;

class ApuSoundChannelBase {
public:
  virtual ~ApuSoundChannelBase() = default;

  virtual void Reset();
  virtual void Restart();

  void UpdateFrequencyTimer();
  void UpdateLengthCounter();

  u8 CalculateDacOutputVolume() const;

  void SetLengthCtrl(u8 val);
  u8 GetLengthCtrl() const;

  bool IsEnabled() const;

protected:
  bool enabled_;

  bool lengthEnabled_;
  u16 lengthCounter_;

  void ResetLengthCounter(u8 lengthSubtract = 0);

  void SetDacEnabled(bool val);
  bool IsDacEnabled() const;

  virtual void UpdateFrequency() = 0;

  virtual u16 GetFrequencyTimerPeriod() const = 0;
  virtual u16 GetMaxLength() const = 0;

  virtual u8 CalculateOutputVolume() const = 0;

private:
  bool dacEnabled_;
  u16 freqTimer_;
};

class ApuEnvelopeChannelBase : public ApuSoundChannelBase {
public:
  virtual ~ApuEnvelopeChannelBase() = default;

  virtual void Reset() override;
  virtual void Restart() override;

  void UpdateEnvelope();

  void SetEnvelopeCtrl(u8 val);
  u8 GetEnvelopeCtrl() const;

protected:
  u8 envTimer_, envVolume_;
  u8 envCtrl_;

  u8 ResetEnvelopeTimer(); // returns the new envelope period
  void ResetEnvelopeVolume();

  u16 GetMaxLength() const override;
};

#endif // SDGBC_APU_CHAN_BASE_H_
