#ifndef SDGBC_TIMER_H_
#define SDGBC_TIMER_H_

#include "types.h"

class Cpu;

class Timer {
public:
  explicit Timer(Cpu& cpu);

  void Reset();
  void Update(unsigned int cycles);

  void ResetDiv();
  u8 GetDiv() const;

  void SetTima(u8 val);
  u8 GetTima() const;

  void SetTma(u8 val);
  u8 GetTma() const;

  void SetTac(u8 val);
  u8 GetTac() const;

private:
  Cpu& cpu_;

  // timer & divider registers & clock counters
  unsigned int divCycles_, timaCycles_;
  u8 div_, tima_, tma_, tac_;

  unsigned int GetTimaFreqInCycles() const;
};

#endif // SDGBC_TIMER_H_
