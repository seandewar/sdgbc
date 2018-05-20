#include "hw/gbc.h"

GbcHardware::GbcHardware()
    : cpu(mmu, dma, joypad), timer(cpu), apu(cpu), ppu(cpu, dma), joypad(cpu),
      serial(cpu), dma(mmu, cpu, ppu), mmu(*this) {}

Gbc::Gbc() : cgbMode_(false) {}

void Gbc::Reset(bool forceDmgMode) {
  cgbMode_ = !forceDmgMode && hw_.cartridge.IsInCgbMode();

  hw_.cpu.Reset(cgbMode_);
  hw_.ppu.Reset(cgbMode_);
  hw_.mmu.Reset(cgbMode_);
  hw_.dma.Reset(cgbMode_);
  hw_.serial.Reset(cgbMode_);
  hw_.apu.Reset();
  hw_.timer.Reset();
  hw_.joypad.Reset();
  hw_.cartridge.Reset();

  // zero-out contents of WRAM and HRAM
  hw_.hram.fill(0x00);

  for (auto& b : hw_.wramBanks) {
    b.fill(0x00);
  }
}

unsigned int Gbc::Update() {
  const auto cycles = hw_.cpu.Update();

  hw_.dma.Update(cycles);
  hw_.apu.Update(cycles);
  hw_.ppu.Update(cycles);
  hw_.timer.Update(cycles);
  hw_.serial.Update(cycles);

  return cycles;
}

RomLoadResult Gbc::LoadCartridgeRomFile(const std::string& filePath,
                                        const std::string& fileName) {
  const auto result = hw_.cartridge.LoadRomFile(filePath, fileName);
  if (result == RomLoadResult::Ok) {
    Reset();
  }

  return result;
}

GbcHardware& Gbc::GetHardware() {
  return hw_;
}

const GbcHardware& Gbc::GetHardware() const {
  return hw_;
}

bool Gbc::IsInCgbMode() const {
  return cgbMode_;
}
