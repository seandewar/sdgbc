#include "hw/cart/cart_ext_mbc5.h"
#include "hw/cart/cart.h"
#include "util.h"

bool Mbc5::ExtInit() {
  if (cart_->GetNumRomBanks() < 1 || cart_->GetNumRomBanks() > 0x200 ||
      cart_->GetNum2KBExtRamBanks() > 64) {
    return false;
  }

  return MbcBase::ExtInit();
}

void Mbc5::ExtRomBank0Write8(u16 loc, u8 val) {
  if (loc < 0x2000) {
    // ext RAM read/write enable switch
    if ((val & 0xf) == 0xa) {
      ramEnabled_ = true;
    } else if (val == 0) {
      ramEnabled_ = false;
    }
  } else if (loc < 0x3000) {
    // lower 8 bits of the 9-bit ROM bank number
    romBankNum_ = util::SetLo8(romBankNum_, val);
  } else {
    // high bit of the 9-bit ROM bank number
    romBankNum_ = util::SetHi8(romBankNum_, val & 1);
  }
}

void Mbc5::ExtRomBankXWrite8(u16 loc, u8 val) {
  if (loc < 0x2000) {
    // 4-bit RAM bank number
    ramBankNum_ = val & 0xf;
  }
}
