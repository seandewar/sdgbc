#ifndef SDGBC_GBC_H_
#define SDGBC_GBC_H_

#include "hw/apu/apu.h"
#include "hw/cart/cart.h"
#include "hw/cpu/cpu.h"
#include "hw/dma.h"
#include "hw/joypad.h"
#include "hw/mmu.h"
#include "hw/ppu.h"
#include "hw/serial.h"
#include "hw/timer.h"

struct GbcHardware {
  Cpu cpu;
  Dma dma;
  Ppu ppu;
  Apu apu;
  Mmu mmu;
  Timer timer;
  Serial serial;
  Joypad joypad;

  WorkRamBanks wramBanks;
  HighRam hram;

  Cartridge cartridge;

  GbcHardware();
};

class Gbc {
public:
  Gbc();

  void Reset(bool forceDmgMode = false);
  unsigned int Update(); // returns the amount of CPU cycles spent

  RomLoadResult LoadCartridgeRomFile(const std::string& filePath,
                                     const std::string& fileName = {});

  GbcHardware& GetHardware();
  const GbcHardware& GetHardware() const;

  bool IsInCgbMode() const;

private:
  GbcHardware hw_;
  bool cgbMode_;
};

#endif // SDGBC_GBC_H_
