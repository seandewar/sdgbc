#ifndef SDGBC_APU_CHAN_SQUARE_H_
#define SDGBC_APU_CHAN_SQUARE_H_

#include "hw/apu/apu_chan_base.h"

class ApuSquareChannel : public ApuEnvelopeChannelBase {
public:
  virtual ~ApuSquareChannel() = default;

  virtual void Reset() override;

  void ResetDutyCounter();

  void SetLengthLoadDutyCtrl(u8 val);
  u8 GetLengthLoadDutyCtrl() const;

  void SetFreqLoadLo(u8 val);
  void SetLengthCtrlFreqLoadHi(u8 val);

protected:
  u8 dutyNum_;
  u8 dutyBitIdxCounter_;

  u16 freqLoad_;

  void UpdateFrequency() override;
  u8 CalculateOutputVolume() const override;

  u16 GetFrequencyTimerPeriod() const override;
};

class ApuSquareSweepChannel : public ApuSquareChannel {
public:
  void Reset() override;
  void Restart() override;

  void UpdateSweep();

  void SetSweepCtrl(u8 val);
  u8 GetSweepCtrl() const;

private:
  u8 sweepTimer_;
  u16 sweepShadow_;
  bool sweepEnabled_;
  u8 sweepCtrl_;

  u8 ResetSweepTimer(); // returns the new sweep period
  void ResetSweepShadowFrequency();

  u16 CalculateNewSweepFreq();
  u8 GetSweepShift() const;
};

#endif // SDGBC_APU_CHAN_SQUARE_H_
