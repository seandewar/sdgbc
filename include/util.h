#ifndef SDGBC_UTIL_H_
#define SDGBC_UTIL_H_

#include "types.h"
#include <iostream>
#include <vector>

class Cpu;

namespace util {
  u16 To16(u8 hi, u8 lo);

  u16 SetHi8(u16 val, u8 hi);
  u16 SetLo8(u16 val, u8 lo);

  u8 GetHi8(u16 val);
  u8 GetLo8(u16 val);

  u8 ReverseBits(u8 val);

  // halves the amount of cycles given if the CPU is in double-speed mode.
  // used for hardware components that run at a constant speed regardless of
  // double-speed mode
  unsigned int RescaleCycles(const Cpu& cpu, unsigned int cycles);

  bool ReadBinaryStream(std::istream& is, std::vector<u8>& data,
                        bool resizeToFitData = true);
  bool WriteBinaryStream(std::ostream& os, const std::vector<u8>& data);
}

#endif // SDGBC_UTIL_H_
