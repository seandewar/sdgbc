#ifndef SDGBC_CART_H_
#define SDGBC_CART_H_

#include "hw/cart/cart_ext_meta.h"
#include "hw/memory.h"
#include "types.h"
#include <memory>
#include <string>
#include <vector>

enum class RomLoadResult {
  Ok,
  ReadError,
  InvalidSize,
  InvalidExtension,
  Unsupported
};

class ICartridgeExtension;

class Cartridge {
public:
  static std::string GetRomLoadResultAsMessage(RomLoadResult result);

  Cartridge();
  explicit Cartridge(const Cartridge& other) = delete;
  explicit Cartridge(Cartridge&& other) = default;
  ~Cartridge();

  Cartridge& operator=(const Cartridge& other) = delete;
  Cartridge& operator=(Cartridge&& other) = default;

  void Clear(); // clears cartridge ROM and resets
  void Reset(); // only clears cartridge RAM/other volatile memory

  RomLoadResult LoadRomFile(const std::string& filePath,
                            const std::string& fileName = {});

  // SaveBatteryExtRam() and LoadBatteryExtRam() return true if the operation
  // was successful OR if the cartridge doesn't need to save RAM (no battery, no
  // RAM etc.) -- false otherwise
  bool SaveBatteryExtRam(const std::string& filePath = {}) const;
  bool LoadBatteryExtRam(const std::string& filePath = {});

  void RomBank0Write8(u16 loc, u8 val);
  u8 RomBank0Read8(u16 loc) const;

  void RomBankXWrite8(u16 loc, u8 val);
  u8 RomBankXRead8(u16 loc) const;

  void RamWrite8(u16 loc, u8 val);
  u8 RamRead8(u16 loc) const;

  std::string GetRomFilePath() const;
  std::string GetRomFileName() const;

  bool IsRomLoaded() const;
  const std::vector<u8>& GetRomData() const;

  u16 GetNumRomBanks() const;
  u16 GetNum2KBExtRamBanks() const;

  u8 GetExtensionId() const;
  const CartridgeExtMeta& GetExtensionMeta() const;

  bool IsInCgbMode() const;

private:
  std::string romFilePath_, romFileName_;
  bool isRomLoaded_;

  std::vector<u8> romData_;
  u16 numRomBanks_, num2KBExtRamBanks_;
  bool cgbMode_;

  std::unique_ptr<ICartridgeExtension> extension_;
  u8 extensionId_;

  RomLoadResult ParseRomHeader(Cartridge& newCart);
  RomLoadResult ParseExtensions(Cartridge& newCart);
};

#endif // SDGBC_CART_H_
