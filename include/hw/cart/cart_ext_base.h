#ifndef SDGBC_CART_EXT_BASE_H_
#define SDGBC_CART_EXT_BASE_H_

#include "types.h"
#include <iostream>
#include <vector>

class Cartridge;

class ICartridgeExtension {
public:
  virtual ~ICartridgeExtension() = default;

  virtual void ExtSetCartridge(const Cartridge* cart) = 0;
  virtual bool ExtInit() = 0;

  virtual void ExtReset() = 0;

  virtual void ExtRomBank0Write8(u16 loc, u8 val) = 0;
  virtual u8 ExtRomBank0Read8(u16 loc) const = 0;

  virtual void ExtRomBankXWrite8(u16 loc, u8 val) = 0;
  virtual u8 ExtRomBankXRead8(u16 loc) const = 0;

  virtual void ExtRamWrite8(u16 loc, u8 val) = 0;
  virtual u8 ExtRamRead8(u16 loc) const = 0;

  virtual bool ExtSaveRam(std::ostream& os) = 0;
  virtual bool ExtLoadRam(std::istream& is) = 0;
};

class CartridgeExtensionBase : public ICartridgeExtension {
public:
  CartridgeExtensionBase();
  virtual ~CartridgeExtensionBase() = default;

  void ExtSetCartridge(const Cartridge* cart) override;

  virtual void ExtRomBank0Write8(u16 loc, u8 val) override;
  virtual u8 ExtRomBank0Read8(u16 loc) const override;

  virtual void ExtRomBankXWrite8(u16 loc, u8 val) override;
  virtual u8 ExtRomBankXRead8(u16 loc) const override;

protected:
  const Cartridge* cart_;
};

class RamExtensionBase : public CartridgeExtensionBase {
public:
  virtual ~RamExtensionBase() = default;

  virtual bool ExtInit() override;
  virtual void ExtReset() override;

  virtual void ExtRamWrite8(u16 loc, u8 val) override;
  virtual u8 ExtRamRead8(u16 loc) const override;

  bool ExtSaveRam(std::ostream& os) override;
  bool ExtLoadRam(std::istream& is) override;

protected:
  std::vector<u8> ramData_;
  u16 ramBankNum_;
  bool ramEnabled_;

  void RamBankXWrite8(u16 loc, u8 val, u16 bankNum);
  u8 RamBankXRead8(u16 loc, u16 bankNum) const;
};

class MbcBase : public RamExtensionBase {
public:
  virtual ~MbcBase() = default;

  virtual void ExtReset() override;

  virtual u8 ExtRomBank0Read8(u16 loc) const override;
  virtual u8 ExtRomBankXRead8(u16 loc) const override;

protected:
  u16 romBankNum_;

  u8 RomBankXRead8(u16 loc, u16 bankNum) const;
};

#endif // SDGBC_CART_EXT_BASE_H_
