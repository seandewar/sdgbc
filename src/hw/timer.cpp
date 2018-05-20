#include "hw/cpu/cpu.h"
#include "hw/timer.h"
#include <cassert>

// calculated as (non-double speed frequency in Hz) / (timer frequency in Hz)
enum TimerFreqInCycles : unsigned int {
  kTimerFreq262144HzCycles = 16,
  kTimerFreq65536HzCycles  = 64,
  kTimerFreq16384HzCycles  = 256,
  kTimerFreq4096HzCycles   = 1024
};

Timer::Timer(Cpu& cpu) : cpu_(cpu) {}

void Timer::Reset() {
  // initial register & clock counter values
  divCycles_ = timaCycles_ = 0xff;

  div_ = 0xd3;
  tima_ = tma_ = 0x00;
  tac_ = 0xf8;
}

void Timer::Update(unsigned int cycles) {
  divCycles_ += cycles;

  // DIV increments at 16384 Hz, which takes 256 clock cycles
  div_ += (divCycles_ / kTimerFreq16384HzCycles) & 0xff;
  divCycles_ %= kTimerFreq16384HzCycles;

  // increment the timer register (TIMA) if it is running (bit 2 set of TAC)
  if (tac_ & 4) {
    timaCycles_ += cycles;

    // TIMA increments using the frequency value set in TAC
    const auto timaFreq = GetTimaFreqInCycles();
    const auto newTima = tima_ + (timaCycles_ / timaFreq);

    if (newTima > 0xff) {
      // TIMA is going to overflow. set it to TMA while accounting for the
      // additional cycles after the change, and request int $50
      cpu_.IntfRequest(kCpuInterrupt0x50);
      tima_ = (newTima % (0x100 - tma_)) & 0xff;
    } else {
      tima_ = newTima & 0xff;
    }

    timaCycles_ %= timaFreq;
  }
}

unsigned int Timer::GetTimaFreqInCycles() const {
  // determined by the first 2 bits of TAC
  switch (tac_ & 3) {
    case 0:
      return kTimerFreq4096HzCycles;
    case 1:
      return kTimerFreq262144HzCycles;
    case 2:
      return kTimerFreq65536HzCycles;
    default: assert(!"unknown TIMA freq value! this should be unreachable");
    case 3:
      return kTimerFreq16384HzCycles;
  }
}

void Timer::ResetDiv() {
  divCycles_ = div_ = 0;
}

u8 Timer::GetDiv() const {
  return div_;
}

void Timer::SetTima(u8 val) {
  tima_ = val;
}

u8 Timer::GetTima() const {
  return tima_;
}

void Timer::SetTma(u8 val) {
  tma_ = val;
}

u8 Timer::GetTma() const {
  return tma_;
}

void Timer::SetTac(u8 val) {
  tac_ = val | 0xf8;
  timaCycles_ = 0;
}

u8 Timer::GetTac() const {
  return tac_;
}
