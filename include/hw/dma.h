#ifndef SDGBC_DMA_H_
#define SDGBC_DMA_H_

#include "types.h"

class Mmu;
class Cpu;
class Ppu;

enum class NdmaStatus {
  Inactive,
  HdmaWaitingForHBlank,
  HdmaFinishedBlock,
  HdmaInProgress,
  GdmaInProgress
};

enum class OamDmaStatus {
  Inactive,
  InProgress
};

class Dma {
public:
  explicit Dma(const Mmu& mmu, const Cpu& cpu, Ppu& ppu);

  void Reset(bool cgbMode);
  void Update(unsigned int cycles);

  void StartOamDmaTransfer(u8 sourceLocHi);
  u8 GetOamDmaSourceLocHi() const;

  void SetNdma5(u8 val);
  u8 GetNdma5() const;

  void SetNdmaSourceLocHi(u8 hi);
  void SetNdmaSourceLocLo(u8 lo);
  void SetNdmaDestLocHi(u8 hi);
  void SetNdmaDestLocLo(u8 lo);

  NdmaStatus GetNdmaStatus() const;
  bool IsNdmaInProgress() const;
  bool IsHdmaInProgress() const;
  bool IsGdmaInProgress() const;

  OamDmaStatus GetOamDmaStatus() const;
  bool IsOamDmaInProgress() const;

  bool IsInCgbMode() const;

private:
  const Mmu& mmu_;
  const Cpu& cpu_;
  Ppu& ppu_;

  bool cgbMode_;

  OamDmaStatus oamDmaStatus_;
  u8 oamDmaSourceLocHi_;
  unsigned int oamDmaCyclesLeft_;

  NdmaStatus ndmaStatus_;
  u16 ndmaReadLoc_, ndmaVramWriteLoc_;
  u8 ndmaNumBlocksLeft_;

  unsigned int gdmaCyclesLeft_, hdmaBlockCyclesLeft_;

  void HandleOamDmaUpdate(unsigned int cycles);
  void HandleGdmaUpdate(unsigned int cycles);
  void HandleHdmaUpdate(unsigned int cycles);

  void UpdateHdmaHBlankState();

  void DoNdmaTransfer(u8 maxNumBlocks);
  void DoOamDmaTransfer();

  bool IsHdmaEnabled() const;
  bool IsNdmaEnabled() const;
};

#endif // SDGBC_DMA_H_
