#ifndef SDGBC_MEMORY_H_
#define SDGBC_MEMORY_H_

#include "types.h"
#include <array>

constexpr u16 kRomBankSize    = 0x4000,
              kExtRamBankSize = 0x2000;

using WorkRamBank = std::array<u8, 0x1000>;
using WorkRamBanks = std::array<WorkRamBank, 8>;

using VideoRamBank = std::array<u8, 0x2000>;
using VideoRamBanks = std::array<VideoRamBank, 2>;

using HighRam = std::array<u8, 0x7f>;

using ObjectAttribMemory = std::array<u8, 0xa0>;
using CgbPaletteMemory = std::array<u8, 0x40>;

using WaveRam = std::array<u8, 0x10>;

#endif // SDGBC_MEMORY_H_
