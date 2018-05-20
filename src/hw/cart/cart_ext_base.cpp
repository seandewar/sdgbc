#include "hw/cart/cart.h"
#include "hw/cart/cart_ext_base.h"
#include "util.h"

CartridgeExtensionBase::CartridgeExtensionBase() : cart_(nullptr) {}

void CartridgeExtensionBase::ExtSetCartridge(const Cartridge* cart) {
  cart_ = cart;
}

// ROM read-only by default
void CartridgeExtensionBase::ExtRomBank0Write8(u16, u8) {}

void CartridgeExtensionBase::ExtRomBankXWrite8(u16, u8) {}

u8 CartridgeExtensionBase::ExtRomBank0Read8(u16 loc) const {
  return cart_->GetRomData()[loc];
}

u8 CartridgeExtensionBase::ExtRomBankXRead8(u16 loc) const {
  return cart_->GetRomData()[kRomBankSize + loc];
}

bool RamExtensionBase::ExtInit() {
  // clear and zero-out RAM to new size
  ramData_.clear();
  ramData_.resize(cart_->GetNum2KBExtRamBanks() * 0x800, 0x00);
  return true;
}

void RamExtensionBase::ExtReset() {
  ramEnabled_ = false;
  ramBankNum_ = 0;

  // zero-out RAM that isn't battery-packed
  if (!cart_->GetExtensionMeta().hasBattery) {
    std::fill(ramData_.begin(), ramData_.end(), 0x00);
  }
}

void RamExtensionBase::RamBankXWrite8(u16 loc, u8 val, u16 bankNum) {
  if (ramEnabled_ && ramData_.size() > 0) {
    const std::size_t dataIndex = (kExtRamBankSize * bankNum + loc)
                                  % ramData_.size();
    ramData_[dataIndex] = val;
  }
}

u8 RamExtensionBase::RamBankXRead8(u16 loc, u16 bankNum) const {
  if (ramEnabled_ && ramData_.size() > 0) {
    const std::size_t dataIndex = (kExtRamBankSize * bankNum + loc)
                                  % ramData_.size();
    return ramData_[dataIndex];
  } else {
    return 0xff;
  }
}

void RamExtensionBase::ExtRamWrite8(u16 loc, u8 val) {
  RamBankXWrite8(loc, val, ramBankNum_);
}

u8 RamExtensionBase::ExtRamRead8(u16 loc) const {
  return RamBankXRead8(loc, ramBankNum_);
}

bool RamExtensionBase::ExtSaveRam(std::ostream& os) {
  return util::WriteBinaryStream(os, ramData_);
}

bool RamExtensionBase::ExtLoadRam(std::istream& is) {
  return util::ReadBinaryStream(is, ramData_, false);
}

void MbcBase::ExtReset() {
  romBankNum_ = cart_->GetNumRomBanks() > 1 ? 1 : 0;
  RamExtensionBase::ExtReset();
}

u8 MbcBase::RomBankXRead8(u16 loc, u16 bankNum) const {
  const std::size_t dataIndex = (kRomBankSize * bankNum + loc)
                                % cart_->GetRomData().size();
  return cart_->GetRomData()[dataIndex];
}

u8 MbcBase::ExtRomBank0Read8(u16 loc) const {
  return RomBankXRead8(loc, 0);
}

u8 MbcBase::ExtRomBankXRead8(u16 loc) const {
  return RomBankXRead8(loc, romBankNum_);
}
