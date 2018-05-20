#include "hw/cart/cart_ext_mbc1.h"
#include "hw/cart/cart.h"

bool Mbc1::ExtInit() {
  if (cart_->GetNumRomBanks() <= 1 || cart_->GetNumRomBanks() > 0x80 ||
      cart_->GetNum2KBExtRamBanks() > 16) {
    return false;
  }

  return MbcBase::ExtInit();
}

void Mbc1::ExtReset() {
  ramBankingMode_ = false;
  MbcBase::ExtReset();
}

void Mbc1::ExtRomBank0Write8(u16 loc, u8 val) {
  if (loc < 0x2000) {
    // ext RAM read/write enable switch
    if ((val & 0xf) == 0xa) {
      ramEnabled_ = true;
    } else if (val == 0) {
      ramEnabled_ = false;
    }
  } else {
    // ROM bank bits 0-4 select
    romBankNum_ = (romBankNum_ & 0x60) | (val & 0x1f);

    // if bits 0-4 are now 0, make it 1 instead. although this stops bank 0
    // from being selected, it also stops banks $20, $40 and $60 from being
    // selected (MBC1 design flaw!)
    if ((romBankNum_ & 0x1f) == 0) {
      ++romBankNum_;
    }
  }
}

void Mbc1::ExtRomBankXWrite8(u16 loc, u8 val) {
  if (loc < 0x2000) {
    // ROM bank number bits 5-6 select OR RAM bank number select. the number
    // that is affected depends on the current ROM/RAM banking mode
    if (ramBankingMode_) {
      ramBankNum_ = val & 3;
    } else {
      romBankNum_ = ((val & 3) << 5) | (romBankNum_ & 0x1f);
    }
  } else {
    // ROM/RAM banking mode select
    ramBankingMode_ = (val & 1) != 0;
  }
}

u8 Mbc1::ExtRomBankXRead8(u16 loc) const {
  if (ramBankingMode_) {
    // can only access ROM banks 00-1F in RAM banking mode
    return RomBankXRead8(loc, romBankNum_ % 0x20);
  } else {
    return MbcBase::ExtRomBankXRead8(loc);
  }
}

void Mbc1::ExtRamWrite8(u16 loc, u8 val) {
  if (ramBankingMode_) {
    RamExtensionBase::ExtRamWrite8(loc, val);
  } else {
    // can only access RAM bank 0 in ROM banking mode
    RamBankXWrite8(loc, val, 0);
  }
}

u8 Mbc1::ExtRamRead8(u16 loc) const {
  if (ramBankingMode_) {
    return RamExtensionBase::ExtRamRead8(loc);
  } else {
    // can only access RAM bank 0 in ROM banking mode
    return RamBankXRead8(loc, 0);
  }
}
