#include "hw/cpu/cpu.h"
#include "hw/dma.h"
#include "hw/mmu.h"
#include "hw/ppu.h"
#include "util.h"
#include <tuple>

constexpr u8 kNdmaBlockSize = 16;

constexpr auto kNdmaCyclesPerBlock = 32u,
               kOamDmaTotalCycles  = 648u;

Dma::Dma(const Mmu& mmu, const Cpu& cpu, Ppu& ppu)
    : mmu_(mmu), cpu_(cpu), ppu_(ppu) {}

void Dma::Reset(bool cgbMode) {
  cgbMode_ = cgbMode;

  oamDmaStatus_ = OamDmaStatus::Inactive;
  oamDmaSourceLocHi_ = 0x00;

  ndmaStatus_ = NdmaStatus::Inactive;
  ndmaReadLoc_ = ndmaVramWriteLoc_ = 0x0000;
  ndmaNumBlocksLeft_ = 0;
}

void Dma::Update(unsigned int cycles) {
  // DMAs only happen while the CPU isn't suspended
  if (cpu_.GetStatus() != CpuStatus::Running) {
    return;
  }

  HandleGdmaUpdate(cycles);
  HandleHdmaUpdate(cycles);
  HandleOamDmaUpdate(cycles);
}

void Dma::DoNdmaTransfer(u8 maxNumBlocks) {
  const u8 numBlocks = std::min(maxNumBlocks, ndmaNumBlocksLeft_);

  for (u8 i = 0; i < numBlocks; ++i) {
    for (u8 j = 0; j < kNdmaBlockSize; ++j) {
      // don't copy past the bounds of the VRAM bank
      if (ndmaVramWriteLoc_ + j >= std::tuple_size<VideoRamBank>::value) {
        break;
      }

      ppu_.VramWrite8(ndmaVramWriteLoc_ + j, mmu_.Read8(ndmaReadLoc_ + j));
    }

    ndmaVramWriteLoc_ += kNdmaBlockSize;
    ndmaReadLoc_ += kNdmaBlockSize;
    --ndmaNumBlocksLeft_;
  }
}

void Dma::HandleGdmaUpdate(unsigned int cycles) {
  if (!IsGdmaInProgress()) {
    return;
  }

  // GDMA speed does not scale with double speed mode
  cycles = util::RescaleCycles(cpu_, cycles);

  if (cycles >= gdmaCyclesLeft_) {
    // GDMA time finished
    DoNdmaTransfer(ndmaNumBlocksLeft_);
    ndmaStatus_ = NdmaStatus::Inactive;

    gdmaCyclesLeft_ = 0;
  } else {
    gdmaCyclesLeft_ -= cycles;
  }
}

void Dma::HandleHdmaUpdate(unsigned int cycles) {
  if (!IsHdmaEnabled()) {
    return;
  }

  UpdateHdmaHBlankState();
  if (!IsHdmaInProgress()) {
    return;
  }

  // HDMA speed does not scale with double speed mode
  cycles = util::RescaleCycles(cpu_, cycles);

  if (cycles >= hdmaBlockCyclesLeft_) {
    // HDMA block transfer time finished
    DoNdmaTransfer(1);
    ndmaStatus_ = (ndmaNumBlocksLeft_ <= 0 ? NdmaStatus::Inactive
                                           : NdmaStatus::HdmaFinishedBlock);
    hdmaBlockCyclesLeft_ = 0;
  } else {
    hdmaBlockCyclesLeft_ -= cycles;
  }
}

void Dma::UpdateHdmaHBlankState() {
  if (ndmaStatus_ == NdmaStatus::HdmaWaitingForHBlank &&
      ppu_.GetScreenMode() == PpuScreenMode::HBlank &&
      ppu_.IsLcdOn()) {
    // we can now start the HBlank block transfer
    hdmaBlockCyclesLeft_ = kNdmaCyclesPerBlock + util::RescaleCycles(cpu_, 4);
    ndmaStatus_ = NdmaStatus::HdmaInProgress;
  } else if (ndmaStatus_ == NdmaStatus::HdmaFinishedBlock &&
             ppu_.GetScreenMode() != PpuScreenMode::HBlank) {
    // HBlank finished, wait for the next one
    ndmaStatus_ = NdmaStatus::HdmaWaitingForHBlank;
  }
}

void Dma::DoOamDmaTransfer() {
  for (u8 i = 0; i < std::tuple_size<ObjectAttribMemory>::value; ++i) {
    ppu_.OamWrite8(i, mmu_.Read8(util::To16(oamDmaSourceLocHi_, i)), true);
  }
}

void Dma::HandleOamDmaUpdate(unsigned int cycles) {
  if (!IsOamDmaInProgress()) {
    return;
  }

  if (cycles >= oamDmaCyclesLeft_) {
    // OAM DMA transfer time finished
    DoOamDmaTransfer();
    oamDmaStatus_ = OamDmaStatus::Inactive;

    oamDmaCyclesLeft_ = 0;
  } else {
    oamDmaCyclesLeft_ -= cycles;
  }
}

void Dma::StartOamDmaTransfer(u8 sourceLocHi) {
  if (sourceLocHi <= 0xdf) {
    oamDmaSourceLocHi_ = sourceLocHi;
    oamDmaCyclesLeft_ = kOamDmaTotalCycles;
    oamDmaStatus_ = OamDmaStatus::InProgress;
  }
}

u8 Dma::GetOamDmaSourceLocHi() const {
  return oamDmaSourceLocHi_;
}

void Dma::SetNdma5(u8 val) {
  if (!cgbMode_) {
    return;
  }

  if (IsHdmaEnabled() && !(val & 0x80)) {
    // HDMA disable requested
    ndmaStatus_ = NdmaStatus::Inactive;
  } else {
    ndmaNumBlocksLeft_ = (val & 0x7f) + 1;

    if (val & 0x80) {
      // HDMA start/restart requested
      ndmaStatus_ = NdmaStatus::HdmaWaitingForHBlank;
    } else {
      // GDMA requested
      gdmaCyclesLeft_ = (kNdmaCyclesPerBlock * ndmaNumBlocksLeft_)
                        + util::RescaleCycles(cpu_, 4);
      ndmaStatus_ = NdmaStatus::GdmaInProgress;
    }
  }
}

u8 Dma::GetNdma5() const {
  // will be $FF when NDMA transfer finished
  return (!IsNdmaEnabled() ? 0x80 : 0) | ((ndmaNumBlocksLeft_ - 1) & 0x7f);
}

void Dma::SetNdmaSourceLocHi(u8 hi) {
  ndmaReadLoc_ = util::SetHi8(ndmaReadLoc_, hi);
}

void Dma::SetNdmaSourceLocLo(u8 lo) {
  // lowest 4 bits ignored
  ndmaReadLoc_ = util::SetLo8(ndmaReadLoc_, lo & 0xf0);
}

void Dma::SetNdmaDestLocHi(u8 hi) {
  // highest 3 bits ignored
  ndmaVramWriteLoc_ = util::SetHi8(ndmaVramWriteLoc_, hi & 0x1f);
}

void Dma::SetNdmaDestLocLo(u8 lo) {
  // lowest 4 bits ignored
  ndmaVramWriteLoc_ = util::SetLo8(ndmaVramWriteLoc_, lo & 0xf0);
}

bool Dma::IsNdmaInProgress() const {
  return IsHdmaInProgress() || IsGdmaInProgress();
}

bool Dma::IsHdmaInProgress() const {
  return ndmaStatus_ == NdmaStatus::HdmaInProgress;
}

bool Dma::IsGdmaInProgress() const {
  return ndmaStatus_ == NdmaStatus::GdmaInProgress;
}

bool Dma::IsOamDmaInProgress() const {
  return oamDmaStatus_ == OamDmaStatus::InProgress;
}

bool Dma::IsHdmaEnabled() const {
  return ndmaStatus_ == NdmaStatus::HdmaWaitingForHBlank ||
         ndmaStatus_ == NdmaStatus::HdmaFinishedBlock ||
         IsHdmaInProgress();
}

bool Dma::IsNdmaEnabled() const {
  return IsHdmaEnabled() || IsGdmaInProgress();
}

OamDmaStatus Dma::GetOamDmaStatus() const {
  return oamDmaStatus_;
}

NdmaStatus Dma::GetNdmaStatus() const {
  return ndmaStatus_;
}

bool Dma::IsInCgbMode() const {
  return cgbMode_;
}
