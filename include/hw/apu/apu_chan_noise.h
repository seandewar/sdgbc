#ifndef SDGBC_APU_CHAN_NOISE_H_
#define SDGBC_APU_CHAN_NOISE_H_

#include "hw/apu/apu_chan_base.h"

class ApuNoiseChannel : public ApuEnvelopeChannelBase {
public:
  void Reset() override;
  void Restart() override;

  void SetLengthLoad(u8 val);

  void SetPolynomialCtrl(u8 val);
  u8 GetPolynomialCtrl() const;

private:
  u8 polyCtrl_;
  u16 linearShift_;

  void UpdateFrequency() override;
  u8 CalculateOutputVolume() const override;

  u16 GetFrequencyTimerPeriod() const override;
};

#endif // SDGBC_APU_CHAN_NOISE_H_
