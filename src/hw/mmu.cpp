#include "hw/gbc.h"
#include <cassert>

Mmu::Mmu(GbcHardware& hw) : hw_(hw) {}

void Mmu::Reset(bool cgbMode) {
  cgbMode_ = cgbMode;
  svbk_ = cgbMode_ ? 0xf8 : 0xff;
}

u8 Mmu::GetWramBankIndex() const {
  return cgbMode_ ? std::max(svbk_ & 7, 1) : 1;
}

bool Mmu::IsInCgbMode() const {
  return cgbMode_;
}

void Mmu::Write8(u16 loc, u8 val) {
  if (loc < 0x4000) {
    // cartridge ROM bank 0
    hw_.cartridge.RomBank0Write8(loc, val);
  } else if (loc < 0x8000) {
    // cartridge switchable ROM bank 0-N
    hw_.cartridge.RomBankXWrite8(loc - 0x4000, val);
  } else if (loc < 0xa000) {
    // VRAM switchable bank 0-1
    hw_.ppu.VramWrite8(loc - 0x8000, val);
  } else if (loc < 0xc000) {
    // external cartridge RAM
    hw_.cartridge.RamWrite8(loc - 0xa000, val);
  } else if (loc < 0xd000) {
    // WRAM fixed bank 0
    hw_.wramBanks[0][loc & 0xfff] = val;
  } else if (loc < 0xe000) {
    // WRAM switchable bank 1-7
    hw_.wramBanks[GetWramBankIndex()][loc & 0xfff] = val;
  } else if (loc < 0xfe00) {
    // echo RAM (same as $C000 to $DDFF)
    Write8(loc - 0x2000, val);
  } else if (loc < 0xfea0) {
    // OAM
    hw_.ppu.OamWrite8(loc - 0xfe00, val);
  } else if (loc < 0xff00) {
    // unusable area. writes have no effect
  } else if (loc >= 0xff30 && loc < 0xff40) {
    // wave RAM
    hw_.apu.WriteWaveRam8(loc & 0xf, val);
  } else if (loc < 0xff80) {
    // IO registers
    WriteIoRegister(loc & 0x7f, val);
  } else if (loc < 0xffff) {
    // HRAM
    hw_.hram[loc & 0x7f] = val;
  } else {
    // interrupt enable IO register
    WriteIoRegister(kIoRegisterIdInte, val);
  }
}

u8 Mmu::Read8(u16 loc) const {
  if (loc < 0x4000) {
    // cartridge ROM bank 0
    return hw_.cartridge.RomBank0Read8(loc);
  } else if (loc < 0x8000) {
    // cartridge switchable ROM bank 0-N
    return hw_.cartridge.RomBankXRead8(loc - 0x4000);
  } else if (loc < 0xa000) {
    // VRAM switchable bank 0-1
    return hw_.ppu.VramRead8(loc - 0x8000);
  } else if (loc < 0xc000) {
    // external cartridge RAM
    return hw_.cartridge.RamRead8(loc - 0xa000);
  } else if (loc < 0xd000) {
    // WRAM fixed bank 0
    return hw_.wramBanks[0][loc & 0xfff];
  } else if (loc < 0xe000) {
    // WRAM switchable bank 1-7
    return hw_.wramBanks[GetWramBankIndex()][loc & 0xfff];
  } else if (loc < 0xfe00) {
    // echo RAM (same as $C000 to $DDFF)
    return Read8(loc - 0x2000);
  } else if (loc < 0xfea0) {
    // OAM
    return hw_.ppu.OamRead8(loc - 0xfe00);
  } else if (loc < 0xff00) {
    // unusable area
    return 0xff;
  } else if (loc >= 0xff30 && loc < 0xff40) {
    // wave RAM - reads return the last value written to wave RAM
    return hw_.apu.GetWaveRamLastWritten8();
  } else if (loc < 0xff80) {
    // IO registers
    return ReadIoRegister(loc & 0x7f);
  } else if (loc < 0xffff) {
    // HRAM
    return hw_.hram[loc & 0x7f];
  } else {
    // interrupt enable IO register
    return ReadIoRegister(kIoRegisterIdInte);
  }
}

void Mmu::WriteIoRegister(u8 regId, u8 val) {
  switch (regId) {
    // serial data transfer registers
    case kIoRegisterIdSb:
      hw_.serial.SetSb(val);
      break;
    case kIoRegisterIdSc:
      hw_.serial.SetSc(val);
      break;

    // joypad register
    case kIoRegisterIdJoyp:
      hw_.joypad.SetJoyp(val);
      break;

    // CPU interrupt registers
    case kIoRegisterIdIntf:
      hw_.cpu.SetIntf(val);
      break;
    case kIoRegisterIdInte:
      hw_.cpu.SetInte(val);
      break;

    // CPU double-speed prepare switch
    case kIoRegisterIdKey1:
      hw_.cpu.SetKey1(val);
      break;

    // APU channel 1 registers
    case kIoRegisterIdNr10:
      hw_.apu.WriteCh1Register(ApuCh1Register::SweepCtrl, val);
      break;
    case kIoRegisterIdNr11:
      hw_.apu.WriteCh1Register(ApuCh1Register::LengthLoadDutyCtrl, val);
      break;
    case kIoRegisterIdNr12:
      hw_.apu.WriteCh1Register(ApuCh1Register::EnvelopeCtrl, val);
      break;
    case kIoRegisterIdNr13:
      hw_.apu.WriteCh1Register(ApuCh1Register::FreqLoadLo, val);
      break;
    case kIoRegisterIdNr14:
      hw_.apu.WriteCh1Register(ApuCh1Register::LengthCtrlFreqLoadHi, val);
      break;

    // APU channel 2 registers
    case kIoRegisterIdNr21:
      hw_.apu.WriteCh2Register(ApuCh2Register::LengthLoadDutyCtrl, val);
      break;
    case kIoRegisterIdNr22:
      hw_.apu.WriteCh2Register(ApuCh2Register::EnvelopeCtrl, val);
      break;
    case kIoRegisterIdNr23:
      hw_.apu.WriteCh2Register(ApuCh2Register::FreqLoadLo, val);
      break;
    case kIoRegisterIdNr24:
      hw_.apu.WriteCh2Register(ApuCh2Register::LengthCtrlFreqLoadHi, val);
      break;

    // APU channel 3 registers
    case kIoRegisterIdNr30:
      hw_.apu.WriteCh3Register(ApuCh3Register::DacOnCtrl, val);
      break;
    case kIoRegisterIdNr31:
      hw_.apu.WriteCh3Register(ApuCh3Register::LengthLoad, val);
      break;
    case kIoRegisterIdNr32:
      hw_.apu.WriteCh3Register(ApuCh3Register::VolumeCtrl, val);
      break;
    case kIoRegisterIdNr33:
      hw_.apu.WriteCh3Register(ApuCh3Register::FreqLoadLo, val);
      break;
    case kIoRegisterIdNr34:
      hw_.apu.WriteCh3Register(ApuCh3Register::LengthCtrlFreqLoadHi, val);
      break;

    // APU channel 4 registers
    case kIoRegisterIdNr41:
      hw_.apu.WriteCh4Register(ApuCh4Register::LengthLoad, val);
      break;
    case kIoRegisterIdNr42:
      hw_.apu.WriteCh4Register(ApuCh4Register::EnvelopeCtrl, val);
      break;
    case kIoRegisterIdNr43:
      hw_.apu.WriteCh4Register(ApuCh4Register::PolynomialCtrl, val);
      break;
    case kIoRegisterIdNr44:
      hw_.apu.WriteCh4Register(ApuCh4Register::LengthCtrl, val);
      break;

    // APU control registers
    case kIoRegisterIdNr50:
      hw_.apu.SetChannelCtrl(val);
      break;
    case kIoRegisterIdNr51:
      hw_.apu.SetOutputCtrl(val);
      break;
    case kIoRegisterIdNr52:
      hw_.apu.SetOnCtrl(val);
      break;

    // timer & divider registers
    case kIoRegisterIdDiv:
      hw_.timer.ResetDiv();
      break;
    case kIoRegisterIdTima:
      hw_.timer.SetTima(val);
      break;
    case kIoRegisterIdTma:
      hw_.timer.SetTma(val);
      break;
    case kIoRegisterIdTac:
      hw_.timer.SetTac(val);
      break;

    // LCD control & status registers
    case kIoRegisterIdLcdc:
      hw_.ppu.SetLcdc(val);
      break;
    case kIoRegisterIdStat:
      hw_.ppu.SetStat(val);
      break;

    // LCD position & scrolling registers
    case kIoRegisterIdScy:
      hw_.ppu.SetScy(val);
      break;
    case kIoRegisterIdScx:
      hw_.ppu.SetScx(val);
      break;
    case kIoRegisterIdLyc:
      hw_.ppu.SetLyc(val);
      break;
    case kIoRegisterIdWy:
      hw_.ppu.SetWy(val);
      break;
    case kIoRegisterIdWx:
      hw_.ppu.SetWx(val);
      break;

    // LCD monochrome palette registers
    case kIoRegisterIdBgp:
      hw_.ppu.SetBgp(val);
      break;
    case kIoRegisterIdObp0:
      hw_.ppu.SetObp0(val);
      break;
    case kIoRegisterIdObp1:
      hw_.ppu.SetObp1(val);
      break;

    // LCD color palette registers
    case kIoRegisterIdBcps:
      hw_.ppu.SetBcps(val);
      break;
    case kIoRegisterIdBcpd:
      hw_.ppu.SetBcpd(val);
      break;
    case kIoRegisterIdOcps:
      hw_.ppu.SetOcps(val);
      break;
    case kIoRegisterIdOcpd:
      hw_.ppu.SetOcpd(val);
      break;

    // LCD VRAM bank register
    case kIoRegisterIdVbk:
      hw_.ppu.SetVbk(val);
      break;

    // new DMA (GDMA/HDMA) registers
    case kIoRegisterIdDma:
      hw_.dma.StartOamDmaTransfer(val);
      break;
    case kIoRegisterIdHdma1:
      hw_.dma.SetNdmaSourceLocHi(val);
      break;
    case kIoRegisterIdHdma2:
      hw_.dma.SetNdmaSourceLocLo(val);
      break;
    case kIoRegisterIdHdma3:
      hw_.dma.SetNdmaDestLocHi(val);
      break;
    case kIoRegisterIdHdma4:
      hw_.dma.SetNdmaDestLocLo(val);
      break;
    case kIoRegisterIdHdma5:
      hw_.dma.SetNdma5(val);
      break;

    // WRAM bank switch registers
    case kIoRegisterIdSvbk:
      svbk_ = cgbMode_ ? val | 0xf8 : 0xff;
      break;
  }
}

u8 Mmu::ReadIoRegister(u8 regId) const {
  switch (regId) {
    // serial data transfer registers
    case kIoRegisterIdSb:
      return hw_.serial.GetSb();
    case kIoRegisterIdSc:
      return hw_.serial.GetSc();

    // joypad register
    case kIoRegisterIdJoyp:
      return hw_.joypad.GetJoyp();

    // CPU interrupt registers
    case kIoRegisterIdIntf:
      return hw_.cpu.GetIntf();
    case kIoRegisterIdInte:
      return hw_.cpu.GetInte();

    // CPU double-speed prepare switch
    case kIoRegisterIdKey1:
      return hw_.cpu.GetKey1();

    // APU channel 1 registers
    case kIoRegisterIdNr10:
      return hw_.apu.ReadCh1Register(ApuCh1Register::SweepCtrl);
    case kIoRegisterIdNr11:
      return hw_.apu.ReadCh1Register(ApuCh1Register::LengthLoadDutyCtrl);
    case kIoRegisterIdNr12:
      return hw_.apu.ReadCh1Register(ApuCh1Register::EnvelopeCtrl);
    case kIoRegisterIdNr14:
      return hw_.apu.ReadCh1Register(ApuCh1Register::LengthCtrlFreqLoadHi);

    // APU channel 2 registers
    case kIoRegisterIdNr21:
      return hw_.apu.ReadCh2Register(ApuCh2Register::LengthLoadDutyCtrl);
    case kIoRegisterIdNr22:
      return hw_.apu.ReadCh2Register(ApuCh2Register::EnvelopeCtrl);
    case kIoRegisterIdNr24:
      return hw_.apu.ReadCh2Register(ApuCh2Register::LengthCtrlFreqLoadHi);

    // APU channel 3 registers
    case kIoRegisterIdNr30:
      return hw_.apu.ReadCh3Register(ApuCh3Register::DacOnCtrl);
    case kIoRegisterIdNr32:
      return hw_.apu.ReadCh3Register(ApuCh3Register::VolumeCtrl);
    case kIoRegisterIdNr34:
      return hw_.apu.ReadCh3Register(ApuCh3Register::LengthCtrlFreqLoadHi);

    // APU channel 4 registers
    case kIoRegisterIdNr42:
      return hw_.apu.ReadCh4Register(ApuCh4Register::EnvelopeCtrl);
    case kIoRegisterIdNr43:
      return hw_.apu.ReadCh4Register(ApuCh4Register::PolynomialCtrl);
    case kIoRegisterIdNr44:
      return hw_.apu.ReadCh4Register(ApuCh4Register::LengthCtrl);

    // APU control registers
    case kIoRegisterIdNr50:
      return hw_.apu.GetChannelCtrl();
    case kIoRegisterIdNr51:
      return hw_.apu.GetOutputCtrl();
    case kIoRegisterIdNr52:
      return hw_.apu.GetOnCtrl();

    // timer & divider registers
    case kIoRegisterIdDiv:
      return hw_.timer.GetDiv();
    case kIoRegisterIdTima:
      return hw_.timer.GetTima();
    case kIoRegisterIdTma:
      return hw_.timer.GetTma();
    case kIoRegisterIdTac:
      return hw_.timer.GetTac();

    // LCD control & status registers
    case kIoRegisterIdLcdc:
      return hw_.ppu.GetLcdc();
    case kIoRegisterIdStat:
      return hw_.ppu.GetStat();

    // LCD position & scrolling registers
    case kIoRegisterIdScy:
      return hw_.ppu.GetScy();
    case kIoRegisterIdScx:
      return hw_.ppu.GetScx();
    case kIoRegisterIdLy:
      return hw_.ppu.GetLy();
    case kIoRegisterIdLyc:
      return hw_.ppu.GetLyc();
    case kIoRegisterIdWy:
      return hw_.ppu.GetWy();
    case kIoRegisterIdWx:
      return hw_.ppu.GetWx();

    // LCD monochrome palette registers
    case kIoRegisterIdBgp:
      return hw_.ppu.GetBgp();
    case kIoRegisterIdObp0:
      return hw_.ppu.GetObp0();
    case kIoRegisterIdObp1:
      return hw_.ppu.GetObp1();

    // LCD color palette registers
    case kIoRegisterIdBcps:
      return hw_.ppu.GetBcps();
    case kIoRegisterIdBcpd:
      return hw_.ppu.GetBcpd();
    case kIoRegisterIdOcps:
      return hw_.ppu.GetOcps();
    case kIoRegisterIdOcpd:
      return hw_.ppu.GetOcpd();

    // LCD VRAM bank register
    case kIoRegisterIdVbk:
      return hw_.ppu.GetVbk();

    // OAM DMA address high byte
    case kIoRegisterIdDma:
      return hw_.dma.GetOamDmaSourceLocHi();

    // new DMA (GDMA/HDMA) start/status register
    case kIoRegisterIdHdma5:
      return hw_.dma.GetNdma5();

    // WRAM bank switch register
    case kIoRegisterIdSvbk:
      return svbk_;

    default:
      return 0xff;
  }
}
