#include "hw/cart/cart.h"
#include "hw/cart/cart_ext_base.h"
#include "hw/cart/cart_ext_mbc1.h"
#include "hw/cart/cart_ext_mbc2.h"
#include "hw/cart/cart_ext_mbc3.h"
#include "hw/cart/cart_ext_mbc5.h"
#include "util.h"
#include <cassert>
#include <fstream>

std::string Cartridge::GetRomLoadResultAsMessage(RomLoadResult result) {
  switch (result) {
    case RomLoadResult::Ok:
      return "ROM image loaded successfully!";

    case RomLoadResult::ReadError:
      return "The ROM image could not be properly read.";

    case RomLoadResult::InvalidSize:
      return "The ROM image has an invalid or unexpected size. The ROM image "
             "might be corrupted!";

    case RomLoadResult::InvalidExtension:
      return "The ROM requires emulation of extended cartridge features that "
             "it cannot support.";

    case RomLoadResult::Unsupported:
      return "The ROM requires emulation of features that are not yet "
             "supported by sdgbc.";

    default: assert(!"unimplemented RomLoadResult message!");
      return "Unknown error!";
  }
}

Cartridge::Cartridge() {
  Clear();
}

Cartridge::~Cartridge() {
  // cartridge object is destroyed when new ROM loaded or program quits
  SaveBatteryExtRam();
}

bool Cartridge::SaveBatteryExtRam(const std::string& filePath) const {
  if (!isRomLoaded_ || !extension_ || !GetExtensionMeta().hasBattery) {
    return true;
  }

  std::ofstream file(filePath.empty() ? romFilePath_ + ".sav" : filePath,
                     std::ios::binary);
  return extension_->ExtSaveRam(file);
}

bool Cartridge::LoadBatteryExtRam(const std::string& filePath) {
  if (!isRomLoaded_ || !extension_ || !GetExtensionMeta().hasBattery) {
    return true;
  }

  std::ifstream file(filePath.empty() ? romFilePath_ + ".sav" : filePath,
                     std::ios::binary);
  return extension_->ExtLoadRam(file);
}

void Cartridge::Clear() {
  Reset();

  romData_.clear();
  romFilePath_.clear();
  romFileName_.clear();

  num2KBExtRamBanks_ = numRomBanks_ = 0;

  extension_.reset();
  extensionId_ = 0x00;

  cgbMode_ = false;
  isRomLoaded_ = false;
}

void Cartridge::Reset() {
  if (extension_) {
    extension_->ExtReset();
  }
}

RomLoadResult Cartridge::LoadRomFile(const std::string& filePath,
                                     const std::string& fileName) {
  {
    // start creating the new cartridge from the ROM data
    Cartridge newCart;

    std::ifstream file(filePath, std::ios::binary);
    if (!util::ReadBinaryStream(file, newCart.romData_)) {
      return RomLoadResult::ReadError;
    }

    const auto parseResult = ParseRomHeader(newCart);
    if (parseResult != RomLoadResult::Ok) {
      return parseResult;
    }

    newCart.romFilePath_ = filePath;
    newCart.romFileName_ = fileName.empty() ? filePath : fileName;
    newCart.isRomLoaded_ = true;

    // loading successful, move to this new cartridge and re-assign cart ptr
    *this = std::move(newCart);
    if (extension_) {
      extension_->ExtSetCartridge(this);
    }
  }

  // old cartridge is destroyed at this point
  Reset();
  LoadBatteryExtRam();
  return RomLoadResult::Ok;
}

RomLoadResult Cartridge::ParseRomHeader(Cartridge& newCart) {
  // make sure that the ROM is big enough to have a header
  if (newCart.romData_.size() < 0x150) {
    return RomLoadResult::InvalidSize;
  }

  // parse ROM size attribute and determine how many ROM banks we should have
  const u8 romSize = newCart.romData_[0x148];
  if (romSize <= 0x07) {
    newCart.numRomBanks_ = 2 << romSize;
  } else if (romSize == 0x52) {
    newCart.numRomBanks_ = 72;
  } else if (romSize == 0x53) {
    newCart.numRomBanks_ = 80;
  } else if (romSize == 0x54) {
    newCart.numRomBanks_ = 96;
  } else {
    return RomLoadResult::InvalidSize;
  }

  // verify that the size of the ROM is what it claims to be
  if (newCart.romData_.size() !=
      static_cast<std::size_t>(newCart.numRomBanks_ * kRomBankSize)) {
    return RomLoadResult::InvalidSize;
  }

  // parse external RAM size attribute
  const u8 exRamSize = newCart.romData_[0x149];
  if (exRamSize == 0x00) {
    newCart.num2KBExtRamBanks_ = 0;
  } else if (exRamSize <= 0x04) {
    newCart.num2KBExtRamBanks_ = 1 << ((exRamSize - 1) * 2);
  } else if (exRamSize == 0x05) {
    newCart.num2KBExtRamBanks_ = 32;
  } else {
    return RomLoadResult::InvalidSize;
  }

  // parse the Game Boy/Game Boy Color compatibility flag
  const u8 cgbFlag = newCart.romData_[0x143];
  newCart.cgbMode_ = cgbFlag == 0x80 || cgbFlag == 0xc0;

  // parse cartridge extensions, if any
  const auto extParseResult = ParseExtensions(newCart);
  if (extParseResult != RomLoadResult::Ok) {
    return extParseResult;
  }

  return RomLoadResult::Ok;
}

RomLoadResult Cartridge::ParseExtensions(Cartridge& newCart) {
  // determine which mapper/hardware extension the cartridge uses, and whether
  // or not we support it
  newCart.extensionId_ = newCart.romData_[0x147];

  const auto metaIt = kCartridgeExtMetas.find(newCart.extensionId_);
  if (metaIt == kCartridgeExtMetas.end()) {
    return RomLoadResult::Unsupported;
  }

  if (!metaIt->second.supportsExRam && newCart.num2KBExtRamBanks_ != 0) {
    // the ROM claims to have external RAM, but the mapper doesn't support it
    return RomLoadResult::InvalidExtension;
  }

  switch (metaIt->second.type) {
    case CartridgeExtType::None:
      newCart.extension_.reset();
      break;
    case CartridgeExtType::Mbc1:
      newCart.extension_ = std::make_unique<Mbc1>();
      break;
    case CartridgeExtType::Mbc2:
      newCart.extension_ = std::make_unique<Mbc2>();
      break;
    case CartridgeExtType::Mbc3:
      // TODO check for RTC if actually going to support it
      newCart.extension_ = std::make_unique<Mbc3>(false);
      break;
    case CartridgeExtType::Mbc5:
      // TODO check for rumble if actually going to support it
      newCart.extension_ = std::make_unique<Mbc5>();
      break;

    default:
      return RomLoadResult::Unsupported;
  }

  // check if this ROM can actually support this extension
  if (newCart.extension_) {
    newCart.extension_->ExtSetCartridge(&newCart);

    if (!newCart.extension_->ExtInit()) {
      return RomLoadResult::InvalidExtension;
    }
  }

  return RomLoadResult::Ok;
}

void Cartridge::RomBank0Write8(u16 loc, u8 val) {
  assert(isRomLoaded_ && loc < kRomBankSize);

  if (extension_) {
    extension_->ExtRomBank0Write8(loc, val);
  }
}

u8 Cartridge::RomBank0Read8(u16 loc) const {
  assert(isRomLoaded_ && loc < kRomBankSize);
  return extension_ ? extension_->ExtRomBank0Read8(loc) : romData_[loc];
}

void Cartridge::RomBankXWrite8(u16 loc, u8 val) {
  assert(isRomLoaded_ && loc < kRomBankSize);

  if (extension_) {
    extension_->ExtRomBankXWrite8(loc, val);
  }
}

u8 Cartridge::RomBankXRead8(u16 loc) const {
  assert(isRomLoaded_ && loc < kRomBankSize);
  return extension_ ? extension_->ExtRomBankXRead8(loc)
                    : romData_[kRomBankSize + loc];
}

void Cartridge::RamWrite8(u16 loc, u8 val) {
  assert(isRomLoaded_ && loc < kExtRamBankSize);

  if (extension_) {
    extension_->ExtRamWrite8(loc, val);
  }
}

u8 Cartridge::RamRead8(u16 loc) const {
  assert(isRomLoaded_ && loc < kExtRamBankSize);
  return extension_ ? extension_->ExtRamRead8(loc) : 0xff;
}

bool Cartridge::IsRomLoaded() const {
  return isRomLoaded_;
}

std::string Cartridge::GetRomFilePath() const {
  return romFilePath_;
}

std::string Cartridge::GetRomFileName() const {
  return romFileName_;
}

const std::vector<u8>& Cartridge::GetRomData() const {
  return romData_;
}

u16 Cartridge::GetNumRomBanks() const {
  return numRomBanks_;
}

u16 Cartridge::GetNum2KBExtRamBanks() const {
  return num2KBExtRamBanks_;
}

u8 Cartridge::GetExtensionId() const {
  return extensionId_;
}

const CartridgeExtMeta& Cartridge::GetExtensionMeta() const {
  const auto metaIt = kCartridgeExtMetas.find(extensionId_);
  assert(metaIt != kCartridgeExtMetas.end());

  return metaIt->second;
}

bool Cartridge::IsInCgbMode() const {
  return cgbMode_;
}
