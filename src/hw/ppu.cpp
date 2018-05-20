#include "hw/cpu/cpu.h"
#include "hw/dma.h"
#include "hw/ppu.h"
#include <algorithm>
#include <tuple>

RgbColor RgbColor::FromLcdIntensities(u8 r, u8 g, u8 b) {
  // a good fast naive intensity to RGB approximation.
  // RGBs can range from 7 to 255, allowing for a near-enough pure black
  assert(r < 0x20 && g < 0x20 && b < 0x20);
  return RgbColor((r * 8) + 7, (g * 8) + 7, (b * 8) + 7);
}

RgbColor::RgbColor() : r(0), g(0), b(0) {}

RgbColor::RgbColor(u8 r, u8 g, u8 b) : r(r), g(g), b(b) {}

constexpr auto kOamMaxSprites    = 40u,
               kScanlineMaxTiles = 21u;

constexpr auto kTileMapWidth  = 32u,
               kTileMapHeight = 32u,
               kTileMapSize   = 256u;

Ppu::Ppu(Cpu& cpu, const Dma& dma)
    : cpu_(cpu), dma_(dma), lcd_(nullptr), limitScanlineSprites_(true),
      enableBg_(true), enableBgWindow_(true), enableSprites_(true) {}

void Ppu::Reset(bool cgbMode) {
  cgbMode_ = cgbMode;
  screenModeCycles_ = 0;

  if (lcd_) {
    lcd_->LcdPower(true);
  }

  // initial register values
  lcdc_ = 0x91;
  stat_ = 0x80;

  scy_ = scx_ = ly_ = lyc_ = wy_ = wx_ = 0x00;

  bgp_ = 0xfc;
  obp0_ = obp1_ = 0x00;

  bcps_ = 0xc8;
  ocps_ = 0xd0;

  vbk_ = 0xfe;

  // zero-out contents of VRAM and OAM
  for (auto& b : vramBanks_) {
    b.fill(0x00);
  }
  oam_.fill(0x00);

  // init contents of BCPD to white (all $FF)
  bcpData_.fill(0xff);
  ocpData_.fill(0xff);
}

void Ppu::Update(unsigned int cycles) {
  if (!IsLcdOn()) {
    return;
  }

  // speed of LCD video hw doesn't scale with double speed mode
  cycles = util::RescaleCycles(cpu_, cycles);
  UpdateScreenMode(cycles);

  // check if we have an LY coincidence at this moment. if we do, set bit 2 in
  // STAT; otherwise, clear it
  stat_ = (ly_ == lyc_ ? stat_ | 4 : stat_ & 0xfb);
}

void Ppu::UpdateScreenMode(unsigned int cycles) {
  screenModeCycles_ += cycles;

  while (screenModeCycles_ >= GetScreenModeMaxCycles()) {
    // account for the extra spent cycles after the mode transition
    screenModeCycles_ -= GetScreenModeMaxCycles();

    switch (GetScreenMode()) {
      case PpuScreenMode::HBlank:
        if (IncrementLy() < kLcdHeightPixels) {
          ChangeScreenMode(PpuScreenMode::SearchingOam);
        } else {
          // finished rendering the last scanline for this frame. now refresh
          // the LCD with our finished frame
          if (lcd_) {
            lcd_->LcdRefresh();
          }

          cpu_.IntfRequest(kCpuInterrupt0x40);
          ChangeScreenMode(PpuScreenMode::VBlank);
        }
        break;

      case PpuScreenMode::VBlank:
        if (IncrementLy() == 0) {
          // finished last VBlank scanline. now start work on the next frame
          ChangeScreenMode(PpuScreenMode::SearchingOam);
        }
        break;

      case PpuScreenMode::SearchingOam:
        ChangeScreenMode(PpuScreenMode::DataTransfer);
        break;

      case PpuScreenMode::DataTransfer:
        RenderScanline();
        ChangeScreenMode(PpuScreenMode::HBlank);
        break;
    }
  }
}

u8 Ppu::IncrementLy() {
  // LY can assume values 0 to 153. >153 wraps around to 0
  SetLy(ly_ < 153 ? ly_ + 1 : 0);
  return ly_;
}

void Ppu::SetLy(u8 val) {
  assert(val <= 153);
  ly_ = val;

  // request interrupt if there is an LY coincidence and STAT bit 6 is set
  if (ly_ == lyc_ && stat_ & 0x40) {
    cpu_.IntfRequest(kCpuInterrupt0x48);
  }
}

void Ppu::ChangeScreenMode(PpuScreenMode mode) {
  stat_ = (stat_ & 0xfc) | (static_cast<u8>(mode) & 3);

  // depending on what mode we've transitioned to, we may need to request an
  // interrupt (if enabled in the STAT register)
  if ((mode == PpuScreenMode::HBlank && stat_ & 0x08) ||
      (mode == PpuScreenMode::VBlank && stat_ & 0x10) ||
      (mode == PpuScreenMode::SearchingOam && stat_ & 0x20)) {
    cpu_.IntfRequest(kCpuInterrupt0x48);
  }
}

PpuScreenMode Ppu::GetScreenMode() const {
  return static_cast<PpuScreenMode>(stat_ & 3);
}

void Ppu::RenderScanline() {
  if (!lcd_) {
    return;
  }

  RenderBufferBgScanline();
  RenderBufferBgWindowScanline();
  RenderBufferSpriteScanline();

  // update LCD scanline
  for (auto x = 0u; x < kLcdWidthPixels; ++x) {
    const auto& bgPixInfo = scanlineBgPixelInfos_[x];
    const auto& spritePixInfo = scanlineSpritePixelInfos_[x];

    lcd_->LcdPutPixel(x, ly_,
        cgbMode_ ? GetCgbPixelColor(bcpData_, bgPixInfo.paletteColorNum)
                 : kDmgPaletteColors[bgPixInfo.paletteColorNum]);

    if (!spritePixInfo.transparent) {
      lcd_->LcdPutPixel(x, ly_,
          cgbMode_ ? GetCgbPixelColor(ocpData_, spritePixInfo.paletteColorNum)
                   : kDmgPaletteColors[spritePixInfo.paletteColorNum]);
    }
  }
}

RgbColor Ppu::GetCgbPixelColor(const CgbPaletteMemory& data,
                               u8 pixelPaletteColorNum) const {
  const u16 cgbColor = util::To16(data[(pixelPaletteColorNum * 2) + 1],
                                  data[pixelPaletteColorNum * 2]);

  return RgbColor::FromLcdIntensities(cgbColor & 0x1f,
                                      (cgbColor >> 5) & 0x1f,
                                      (cgbColor >> 10) & 0x1f);
}

Ppu::BgPixelInfo::BgPixelInfo()
    : paletteColorNum(0), alwaysBehindSprites(true),
      ignoreSpritePriority(false) {}

void Ppu::RenderBufferBgScanline() {
  scanlineBgPixelInfos_.fill(BgPixelInfo());

  // no BG rendered if LCDC bit 0 unset in DMG mode
  if (!enableBg_ || (!cgbMode_ && !(lcdc_ & 1))) {
    return;
  }

  // LCDC bit 3 determines where the BG's tile map is
  const u16 tileMapStartLocOffset = lcdc_ & 0x08 ? 0x1c00 : 0x1800;

  for (auto i = 0u; i < kScanlineMaxTiles; ++i) {
    const auto tileX = i + (scx_ / 8);

    // get the offset location of the tile entry within the tile map
    const u16 tileMapEntryLocOffset = tileMapStartLocOffset
                                      + (tileX % kTileMapWidth)
                                      + (kTileMapWidth * (((ly_ + scy_) / 8)
                                                         % kTileMapHeight));

    // fetch the attribs and pattern line for this tile from VRAM
    const auto tileInfo = GetBgTileInfo(tileMapEntryLocOffset);
    const auto patternLine = GetBgPatternLine(tileInfo.patternNum,
                                              tileInfo.patternBankIndex,
                                              (ly_ + scy_) % 8,
                                              tileInfo.patternFlipX,
                                              tileInfo.patternFlipY);

    // buffer the pixel palette values
    for (auto x = 0u; x < 8; ++x) {
      // the BG map wraps around the screen, and is 256x256 pixels
      RenderBufferBgPixel(tileInfo, GetPatternNumberFromLine(patternLine, x),
                          ((tileX * 8) + x - scx_) % kTileMapSize);
    }
  }
}

void Ppu::RenderBufferBgWindowScanline() {
  // no window rendered if LCDC bit 0 unset in DMG mode or LCDC bit 5 unset
  if (!enableBgWindow_ || (!cgbMode_ && !(lcdc_ & 1)) || !(lcdc_ & 0x20)) {
    return;
  }

  // top-left window screen pixel co-ords. don't bother rendering the window if
  // it's off-screen or not on this scanline
  const int wxActual = wx_ - 7;
  if (wxActual >= static_cast<int>(kLcdWidthPixels) ||
      wy_ >= kLcdHeightPixels || ly_ < wy_) {
    return;
  }

  // LCDC bit 3 determines where the window's tile map is
  const u16 tileMapStartLocOffset = lcdc_ & 0x40 ? 0x1c00 : 0x1800;

  // determine the number of tiles on screen from our window X coordinate and
  // iterate through them for rendering
  const auto numTiles = kScanlineMaxTiles - (wxActual / 8);

  for (auto tileX = 0u; tileX < numTiles; ++tileX) {
    // get the offset location of the tile entry within the 32x32 tile map
    const u16 tileMapEntryLocOffset = tileMapStartLocOffset
                                      + tileX
                                      + (kTileMapWidth * ((ly_ - wy_) / 8));

    // fetch the attribs and pattern line for this tile from VRAM
    const auto tileInfo = GetBgTileInfo(tileMapEntryLocOffset);
    const auto patternLine = GetBgPatternLine(tileInfo.patternNum,
                                              tileInfo.patternBankIndex,
                                              (ly_ - wy_) % 8,
                                              tileInfo.patternFlipX,
                                              tileInfo.patternFlipY);

    // buffer the pixel palette values
    for (auto x = 0u; x < 8; ++x) {
      RenderBufferBgPixel(tileInfo, GetPatternNumberFromLine(patternLine, x),
                          (tileX * 8) + x + wxActual);
    }
  }
}

void Ppu::RenderBufferBgPixel(const BgTileInfo& tileInfo, u8 bgpNum,
                              int pixelX) {
  if (pixelX < 0 || pixelX >= static_cast<int>(kLcdWidthPixels)) {
    return;
  }

  auto& bgPixelInfo = scanlineBgPixelInfos_[pixelX];
  bgPixelInfo.alwaysBehindSprites = bgpNum == 0;
  bgPixelInfo.ignoreSpritePriority = tileInfo.patternPriorityOverSprites;

  if (cgbMode_) {
    // draw using the color palette attribute
    bgPixelInfo.paletteColorNum = (tileInfo.patternCgbPaletteNum * 4)
                                  + bgpNum;
  } else {
    // draw using the monochrome palette register
    bgPixelInfo.paletteColorNum = (bgp_ >> (bgpNum * 2)) & 3;
  }
}

Ppu::SpritePixelInfo::SpritePixelInfo()
    : paletteColorNum(0), transparent(true) {}

void Ppu::RenderBufferSpriteScanline() {
  scanlineSpritePixelInfos_.fill(SpritePixelInfo());

  // no sprites are rendered during OAM DMA or if LCDC bit 1 set
  if (!enableSprites_ || dma_.IsOamDmaInProgress() || !(lcdc_ & 2)) {
    return;
  }

  // enumerate the sprites that are on this scanline.
  // iterate in reverse order so we buffer over sprites with lower priority
  const auto sprites = EnumerateScanlineSprites();

  for (auto rit = sprites.rbegin(); rit != sprites.rend(); ++rit) {
    const auto& sprite = *rit;

    // don't draw hidden sprites
    if (sprite.x == -8 || sprite.x >= static_cast<int>(kLcdWidthPixels)) {
      continue;
    }

    // fetch the pattern line for this sprite from VRAM
    const auto patternLine = GetSpritePatternLine(
        sprite.patternNum,
        cgbMode_ && sprite.attribs & 0x08 ? 1 : 0,
        ly_ - sprite.y,
        (sprite.attribs & 0x20) != 0,
        (sprite.attribs & 0x40) != 0);

    // buffer the pixel palette values
    for (auto x = 0u; x < 8; ++x) {
      if (!RenderBufferSpritePixel(sprite,
                                   GetPatternNumberFromLine(patternLine, x),
                                   sprite.x + x)) {
        break; // no need to render any more pixels in this sprite
      }
    }
  }
}

std::vector<Ppu::Sprite> Ppu::EnumerateScanlineSprites() const {
  // sprites with lower OAM index values will have higher rendering priority
  // (unless we're in DMG mode, where we need to account for X values)
  std::vector<Sprite> sprites;
  sprites.reserve(limitScanlineSprites_ ? 10 : kOamMaxSprites);

  for (auto i = 0u; i < kOamMaxSprites; ++i) {
    // only consider 10 sprites at most (hardware limitation)
    if (limitScanlineSprites_ && sprites.size() >= 10) {
      break;
    }

    const u8 oamLoc = i * 4;

    // minus 16 from attrib 0 and 8 from attrib 1 to get Y & X values of the
    // top-left corner of the sprite
    const int spriteY = oam_[oamLoc] - 16,
              spriteX = oam_[oamLoc + 1] - 8;

    // only consider sprites that are visible on the current scanline
    if (spriteY <= static_cast<int>(ly_) &&
        spriteY + (IsIn8x16SpriteMode() ? 16 : 8) > static_cast<int>(ly_)) {
      sprites.push_back({oamLoc,
                         spriteX, spriteY,
                         oam_[oamLoc + 2], oam_[oamLoc + 3]});
    }
  }

  // DMG mode gives priority to the sprite with the lowest X values
  if (!cgbMode_) {
    std::sort(sprites.begin(), sprites.end(),
              [] (const Sprite& a, const Sprite& b) {
                return (a.x == b.x && a.oamLoc < b.oamLoc) || a.x < b.x;
              });
  }

  return sprites;
}

bool Ppu::RenderBufferSpritePixel(const Sprite& sprite, u8 obpNum, int pixelX) {
  if (pixelX < 0 || obpNum == 0) {
    return true; // pixel off-screen or transparent (pallete color 0)
  } else if (pixelX >= static_cast<int>(kLcdWidthPixels)) {
    return false; // no point buffering more pixels as they'll be off-screen
  }

  // buffer this sprite's pixel if:
  //
  // [BG palette color at this pixel position is 0 (always behind sprites)]
  // OR
  // [LCDC bit 0 unset in CGB mode (acts as a BG master priority switch)]
  // OR
  // [sprite has a higher priority than BG (bit 7 in attribs unset) AND]
  // [BG palette color at this pixel is NOT ignoring sprite priorities ]
  const auto& bgPixelInfo = scanlineBgPixelInfos_[pixelX];
  auto& spritePixelInfo = scanlineSpritePixelInfos_[pixelX];

  if (bgPixelInfo.alwaysBehindSprites || (cgbMode_ && !(lcdc_ & 1)) ||
      (!(sprite.attribs & 0x80) && !bgPixelInfo.ignoreSpritePriority)) {
    spritePixelInfo.transparent = false;

    if (cgbMode_) {
      // draw using the color palette attribute
      spritePixelInfo.paletteColorNum = ((sprite.attribs & 7) * 4) + obpNum;
    } else {
      // draw using the selected monochrome palette register
      const u8 obp = sprite.attribs & 0x10 ? obp1_ : obp0_;
      spritePixelInfo.paletteColorNum = (obp >> (obpNum * 2)) & 3;
    }
  }

  return true;
}

Ppu::BgTileInfo::BgTileInfo(u8 patternNum, u8 patternCgbPaletteNum,
    u8 patternBankIndex, bool patternFlipX, bool patternFlipY,
    bool patternPriorityOverSprites)
    : patternNum(patternNum), patternCgbPaletteNum(patternCgbPaletteNum),
      patternBankIndex(patternBankIndex), patternFlipX(patternFlipX),
      patternFlipY(patternFlipY),
      patternPriorityOverSprites(patternPriorityOverSprites) {}

Ppu::BgTileInfo::BgTileInfo(u8 patternNum)
    : BgTileInfo(patternNum, 0, 0, false, false, false) {}

Ppu::BgTileInfo Ppu::GetBgTileInfo(u16 tileMapEntryLocOffset) const {
  const u8 tilePatternNum = vramBanks_[0][tileMapEntryLocOffset];

  if (cgbMode_) {
    const u8 tileAttribs = vramBanks_[1][tileMapEntryLocOffset];

    return BgTileInfo(tilePatternNum,
                      tileAttribs & 0x07,
                      tileAttribs & 0x08 ? 1 : 0,
                      (tileAttribs & 0x20) != 0, (tileAttribs & 0x40) != 0,
                      (tileAttribs & 0x80) != 0);
  } else {
    // DMG mode doesn't support custom BG tile attribs
    return BgTileInfo(tilePatternNum);
  }
}

std::pair<u8, u8> Ppu::GetPatternLine(
    u16 locOffset, u8 bankIndex, bool flipX) const {
  auto patternLine = std::make_pair(vramBanks_[bankIndex][locOffset],
                                    vramBanks_[bankIndex][locOffset + 1]);

  if (flipX) {
    patternLine.first = util::ReverseBits(patternLine.first);
    patternLine.second = util::ReverseBits(patternLine.second);
  }

  return patternLine;
}

std::pair<u8, u8> Ppu::GetBgPatternLine(
    u8 patternNum, u8 bankIndex, u8 lineNum, bool flipX, bool flipY) const {
  assert(lineNum < 8);

  if (flipY) {
    lineNum = 7 - lineNum;
  }

  // treat pattern number as signed if LCDC bit 4 set, where pattern number 0
  // refers to offset $1000 in the selected VRAM bank
  if (lcdc_ & 0x10) {
    return GetPatternLine(patternNum * 16 + lineNum * 2, bankIndex, flipX);
  } else {
    return GetPatternLine(0x1000 + static_cast<i8>(patternNum) * 16
                                 + lineNum * 2,
                          bankIndex, flipX);
  }
}

std::pair<u8, u8> Ppu::GetSpritePatternLine(
    u8 patternNum, u8 bankIndex, u8 lineNum, bool flipX, bool flipY) const {
  assert(bankIndex < 2 && lineNum < (IsIn8x16SpriteMode() ? 16 : 8));

  if (flipY) {
    lineNum = (IsIn8x16SpriteMode() ? 15 : 7) - lineNum;
  }

  // bit 0 of pattern number ignored in 8x16 mode
  if (IsIn8x16SpriteMode()) {
    patternNum &= 0xfe;
  }

  return GetPatternLine(patternNum * 16 + lineNum * 2, bankIndex, flipX);
}

u8 Ppu::GetPatternNumberFromLine(const std::pair<u8, u8>& line,
                                 u8 numIndex) const {
  assert(numIndex < 8);
  return (((line.second >> (7 - numIndex)) & 1) << 1) |
         ((line.first >> (7 - numIndex)) & 1);
}

bool Ppu::IsIn8x16SpriteMode() const {
  return (lcdc_ & 4) != 0;
}

u8 Ppu::GetVramBankIndex() const {
  return cgbMode_ ? vbk_ & 1 : 0;
}

void Ppu::SetVbk(u8 val) {
  vbk_ = cgbMode_ ? val | 0xfe : 0xfe;
}

u8 Ppu::GetVbk() const {
  return vbk_;
}

void Ppu::VramWrite8(u16 loc, u8 val) {
  assert(loc < std::tuple_size<VideoRamBank>::value);

  // VRAM inaccessible during use
  if (GetScreenMode() != PpuScreenMode::DataTransfer) {
    vramBanks_[GetVramBankIndex()][loc] = val;
  }
}

u8 Ppu::VramRead8(u16 loc) const {
  assert(loc < std::tuple_size<VideoRamBank>::value);

  // VRAM inaccessible during use
  if (GetScreenMode() != PpuScreenMode::DataTransfer) {
    return vramBanks_[GetVramBankIndex()][loc];
  } else {
    return 0xff;
  }
}

bool Ppu::IsOamAccessible() const {
  return GetScreenMode() != PpuScreenMode::DataTransfer &&
         GetScreenMode() != PpuScreenMode::SearchingOam &&
         !dma_.IsOamDmaInProgress();
}

void Ppu::OamWrite8(u16 loc, u8 val, bool oamDmaWrite) {
  assert(loc < oam_.size());

  // OAM inaccessible during use unless this is a DMA write
  if (oamDmaWrite || IsOamAccessible()) {
    oam_[loc] = val;
  }
}

u8 Ppu::OamRead8(u16 loc) const {
  assert(loc < oam_.size());
  return IsOamAccessible() ? oam_[loc] : 0xff;
}

unsigned int Ppu::GetScreenModeMaxCycles() const {
  switch (GetScreenMode()) {
    case PpuScreenMode::HBlank:
      return 204;
    default: assert(!"unknown screen mode!");
    case PpuScreenMode::VBlank:
      return 456;
    case PpuScreenMode::SearchingOam:
      return 80;
    case PpuScreenMode::DataTransfer:
      return 172;
  }
}

bool Ppu::IsLcdOn() const {
  return (lcdc_ & 0x80) != 0;
}

void Ppu::SetLcdc(u8 val) {
  const bool prevLcdOn = IsLcdOn();
  lcdc_ = val;

  // check that the power state actually changed
  const bool newLcdOn = IsLcdOn();
  if (prevLcdOn == newLcdOn) {
    return;
  }

  if (!newLcdOn) {
    // reset screen mode to HBlank at line 0
    ChangeScreenMode(PpuScreenMode::HBlank);
    screenModeCycles_ = 0;
    SetLy(0);
  }

  if (lcd_) {
    lcd_->LcdPower(newLcdOn);
  }
}

u8 Ppu::GetLcdc() const {
  return lcdc_;
}

void Ppu::SetStat(u8 val) {
  stat_ = (val & 0x78) | (stat_ & 0x87); // bits 7,0-2 read-only
}

u8 Ppu::GetStat() const {
  return stat_ & (IsLcdOn() ? 0xff : 0xf8); // bits 0-2 are 0 when LCD off
}

u8 Ppu::GetLy() const {
  return IsLcdOn() ? ly_ : 0;
}

void Ppu::SetLyc(u8 val) {
  lyc_ = val;
}

u8 Ppu::GetLyc() const {
  return lyc_;
}

void Ppu::SetScy(u8 val) {
  scy_ = val;
}

u8 Ppu::GetScy() const {
  return scy_;
}

void Ppu::SetScx(u8 val) {
  scx_ = val;
}

u8 Ppu::GetScx() const {
  return scx_;
}

void Ppu::SetWy(u8 val) {
  wy_ = val;
}

u8 Ppu::GetWy() const {
  return wy_;
}

void Ppu::SetWx(u8 val) {
  wx_ = val;
}

u8 Ppu::GetWx() const {
  return wx_;
}

void Ppu::SetLcd(ILcd* lcd) {
  lcd_ = lcd;
}

void Ppu::SetBgp(u8 val) {
  bgp_ = val;
}

u8 Ppu::GetBgp() const {
  return bgp_;
}

void Ppu::SetObp0(u8 val) {
  obp0_ = val;
}

u8 Ppu::GetObp0() const {
  return obp0_;
}

void Ppu::SetObp1(u8 val) {
  obp1_ = val;
}

u8 Ppu::GetObp1() const {
  return obp1_;
}

void Ppu::WriteCgbPaletteData(CgbPaletteMemory& data, u8& selectReg, u8 val) {
  if (cgbMode_ && GetScreenMode() != PpuScreenMode::DataTransfer) {
    data[selectReg & 0x3f] = val;

    // increment the index value in XCPS (bits 0-5) if bit 7 is set in it
    if (selectReg & 0x80) {
      selectReg = (selectReg & 0xc0) | ((selectReg + 1) & 0x3f);
    }
  }
}

u8 Ppu::ReadCgbPaletteData(const CgbPaletteMemory& data, u8 selectReg) const {
  return GetScreenMode() != PpuScreenMode::DataTransfer ? data[selectReg & 0x3f]
                                                        : 0xff;
}

void Ppu::SetBcps(u8 val) {
  if (cgbMode_) {
    bcps_ = val | 0x40; // bit 6 always set
  }
}

u8 Ppu::GetBcps() const {
  return bcps_;
}

void Ppu::SetBcpd(u8 val) {
  WriteCgbPaletteData(bcpData_, bcps_, val);
}

u8 Ppu::GetBcpd() const {
  return ReadCgbPaletteData(bcpData_, bcps_);
}

void Ppu::SetOcps(u8 val) {
  if (cgbMode_) {
    ocps_ = val | 0x40; // bit 6 always set
  }
}

u8 Ppu::GetOcps() const {
  return ocps_;
}

void Ppu::SetOcpd(u8 val) {
  WriteCgbPaletteData(ocpData_, ocps_, val);
}

u8 Ppu::GetOcpd() const {
  return ReadCgbPaletteData(ocpData_, ocps_);
}

void Ppu::SetScanlineSpritesLimiterEnabled(bool val) {
  limitScanlineSprites_ = val;
}

bool Ppu::IsScanlineSpritesLimiterEnabled() const {
  return limitScanlineSprites_;
}

void Ppu::SetBgRenderEnabled(bool val) {
  enableBg_ = val;
}

bool Ppu::IsBgRenderEnabled() const {
  return enableBg_;
}

void Ppu::SetBgWindowRenderEnabled(bool val) {
  enableBgWindow_ = val;
}

bool Ppu::IsBgWindowRenderEnabled() const {
  return enableBgWindow_;
}

void Ppu::SetSpritesRenderEnabled(bool val) {
  enableSprites_ = val;
}

bool Ppu::IsSpritesRenderEnabled() const {
  return enableSprites_;
}

bool Ppu::IsInCgbMode() const {
  return cgbMode_;
}
