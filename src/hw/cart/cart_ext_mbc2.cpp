#include "hw/cart/cart_ext_mbc2.h"
#include "hw/cart/cart.h"
#include "util.h"

bool Mbc2::ExtInit() {
  if (cart_->GetNumRomBanks() < 1 || cart_->GetNumRomBanks() > 16 ||
      cart_->GetNum2KBExtRamBanks() != 0) {
    return false;
  }

  // MBC2 contains 512x4 bits of RAM.
  // to simplify the implementation, we'll represent this as 512 bytes of RAM,
  // while only writing to the lower 4 bits when ExtRamWrite() is called
  ramData_.resize(0x200);
  ramData_.clear();
  return true;
}

void Mbc2::ExtRomBank0Write8(u16 loc, u8 val) {
  if (loc < 0x2000) {
    if (!(util::GetHi8(loc) & 1)) {
      ramEnabled_ = !ramEnabled_;
    }
  } else {
    romBankNum_ = std::max<u16>(val & 0xf, 1);
  }
}
