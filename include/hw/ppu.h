#ifndef SDGBC_PPU_H_
#define SDGBC_PPU_H_

#include "hw/memory.h"
#include "wxui/wx.h"
#include <vector>

constexpr auto kLcdWidthPixels  = 160u,
               kLcdHeightPixels = 144u;

struct RgbColor {
  static RgbColor FromLcdIntensities(u8 r, u8 g, u8 b);

  u8 r, g, b;

  RgbColor();
  RgbColor(u8 r, u8 g, u8 b);
};

const std::array<RgbColor, 4> kDmgPaletteColors {
  RgbColor(0xff, 0xff, 0xff),
  RgbColor(0x9f, 0x9f, 0x9f),
  RgbColor(0x5f, 0x5f, 0x5f),
  RgbColor(0x00, 0x00, 0x00)
};

class ILcd {
public:
  virtual ~ILcd() = default;

  virtual void LcdPower(bool powerOn) = 0;
  virtual void LcdRefresh() = 0;
  virtual void LcdPutPixel(unsigned int x, unsigned int y,
                           const RgbColor& color) = 0;
};

enum class PpuScreenMode : u8 {
  HBlank = 0,
  VBlank,
  SearchingOam,
  DataTransfer
};

class Cpu;
class Dma;

class Ppu {
public:
  explicit Ppu(Cpu& cpu, const Dma& dma);

  void Reset(bool cgbMode);
  void Update(unsigned int cycles);

  void SetLcd(ILcd* lcd);

  void SetScanlineSpritesLimiterEnabled(bool val);
  bool IsScanlineSpritesLimiterEnabled() const;

  void SetBgRenderEnabled(bool val);
  bool IsBgRenderEnabled() const;

  void SetBgWindowRenderEnabled(bool val);
  bool IsBgWindowRenderEnabled() const;

  void SetSpritesRenderEnabled(bool val);
  bool IsSpritesRenderEnabled() const;

  void VramWrite8(u16 loc, u8 val);
  u8 VramRead8(u16 loc) const;

  void OamWrite8(u16 loc, u8 val, bool oamDmaWrite = false);
  u8 OamRead8(u16 loc) const;

  bool IsLcdOn() const;
  PpuScreenMode GetScreenMode() const;

  void SetVbk(u8 val);
  u8 GetVbk() const;

  void SetLcdc(u8 val);
  u8 GetLcdc() const;

  void SetStat(u8 val);
  u8 GetStat() const;

  u8 GetLy() const;

  void SetLyc(u8 val);
  u8 GetLyc() const;

  void SetScy(u8 val);
  u8 GetScy() const;

  void SetScx(u8 val);
  u8 GetScx() const;

  void SetWy(u8 val);
  u8 GetWy() const;

  void SetWx(u8 val);
  u8 GetWx() const;

  void SetBgp(u8 val);
  u8 GetBgp() const;

  void SetObp0(u8 val);
  u8 GetObp0() const;

  void SetObp1(u8 val);
  u8 GetObp1() const;

  void SetBcps(u8 val);
  u8 GetBcps() const;

  void SetBcpd(u8 val);
  u8 GetBcpd() const;

  void SetOcps(u8 val);
  u8 GetOcps() const;

  void SetOcpd(u8 val);
  u8 GetOcpd() const;

  bool IsInCgbMode() const;

private:
  struct BgTileInfo {
    u8 patternNum;
    u8 patternCgbPaletteNum;
    u8 patternBankIndex;
    bool patternFlipX, patternFlipY;
    bool patternPriorityOverSprites;

    BgTileInfo(u8 patternNum, u8 patternCgbPaletteNum, u8 patternBankIndex,
               bool patternFlipX, bool patternFlipY,
               bool patternPriorityOverSprites);
    BgTileInfo(u8 patternNum);
  };

  struct Sprite {
    u8 oamLoc;
    int x, y;
    u8 patternNum;
    u8 attribs;
  };

  struct BgPixelInfo {
    u8 paletteColorNum;
    bool alwaysBehindSprites;
    bool ignoreSpritePriority;

    BgPixelInfo();
  };

  struct SpritePixelInfo {
    u8 paletteColorNum;
    bool transparent;

    SpritePixelInfo();
  };

  Cpu& cpu_;
  const Dma& dma_;
  ILcd* lcd_;

  VideoRamBanks vramBanks_;
  ObjectAttribMemory oam_;
  CgbPaletteMemory bcpData_, ocpData_;

  std::array<SpritePixelInfo, kLcdWidthPixels> scanlineSpritePixelInfos_;
  std::array<BgPixelInfo, kLcdWidthPixels> scanlineBgPixelInfos_;

  unsigned int screenModeCycles_;
  bool cgbMode_;

  bool limitScanlineSprites_;
  bool enableBg_, enableBgWindow_, enableSprites_;

  // LCD control & status registers
  u8 lcdc_, stat_;

  // LCD position & scrolling registers
  u8 scy_, scx_, ly_, lyc_, wy_, wx_;

  // LCD monochrome palette registers
  u8 bgp_, obp0_, obp1_;

  // LCD color palette registers
  u8 bcps_, ocps_;

  // LCD VRAM bank register
  u8 vbk_;

  void UpdateScreenMode(unsigned int cycles);

  void ChangeScreenMode(PpuScreenMode mode);
  unsigned int GetScreenModeMaxCycles() const;

  u8 IncrementLy();
  void SetLy(u8 val);

  void RenderScanline();
  void RenderBufferSpriteScanline();
  void RenderBufferBgScanline();
  void RenderBufferBgWindowScanline();

  std::vector<Sprite> EnumerateScanlineSprites() const;

  // returns false if there is no need to buffer more pixels in the sprite.
  // true otherwise
  bool RenderBufferSpritePixel(const Sprite& sprite, u8 obpNum, int pixelX);
  void RenderBufferBgPixel(const BgTileInfo& tileInfo, u8 bgpNum, int pixelX);

  RgbColor GetCgbPixelColor(const CgbPaletteMemory& data,
                            u8 pixelPaletteColorNum) const;

  BgTileInfo GetBgTileInfo(u16 tileMapEntryLocOffset) const;

  std::pair<u8, u8> GetPatternLine(
      u16 locOffset, u8 bankIndex, bool flipX) const;
  std::pair<u8, u8> GetBgPatternLine(
      u8 patternNum, u8 bankIndex, u8 lineNum, bool flipX, bool flipY) const;
  std::pair<u8, u8> GetSpritePatternLine(
      u8 patternNum, u8 bankIndex, u8 lineNum, bool flipX, bool flipY) const;

  u8 GetPatternNumberFromLine(const std::pair<u8, u8>& line, u8 numIndex) const;

  void WriteCgbPaletteData(CgbPaletteMemory& data, u8& selectReg, u8 val);
  u8 ReadCgbPaletteData(const CgbPaletteMemory& data, u8 selectReg) const;

  bool IsOamAccessible() const;
  bool IsIn8x16SpriteMode() const;

  u8 GetVramBankIndex() const;
};

#endif // SDGBC_PPU_H_
