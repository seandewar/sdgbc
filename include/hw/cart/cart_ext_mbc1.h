#ifndef SDGBC_CART_EXT_MBC1_H_
#define SDGBC_CART_EXT_MBC1_H_

#include "hw/cart/cart_ext_base.h"

class Mbc1 : public MbcBase {
public:
  bool ExtInit() override;
  void ExtReset() override;

  void ExtRomBank0Write8(u16 loc, u8 val) override;

  void ExtRomBankXWrite8(u16 loc, u8 val) override;
  u8 ExtRomBankXRead8(u16 loc) const override;

  void ExtRamWrite8(u16 loc, u8 val) override;
  u8 ExtRamRead8(u16 loc) const override;

private:
  bool ramBankingMode_;
};

#endif // SDGBC_CART_EXT_MBC1_H_