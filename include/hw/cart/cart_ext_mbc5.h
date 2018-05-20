#ifndef SDGBC_CART_EXT_MBC5_H_
#define SDGBC_CART_EXT_MBC5_H_

#include "hw/cart/cart_ext_base.h"

class Mbc5 : public MbcBase {
public:
  bool ExtInit() override;

  void ExtRomBank0Write8(u16 loc, u8 val) override;
  void ExtRomBankXWrite8(u16 loc, u8 val) override;
};

#endif // SDGBC_CART_EXT_MBC5_H_
