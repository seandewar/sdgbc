#ifndef SDGBC_CART_EXT_META_H_
#define SDGBC_CART_EXT_META_H_

#include "types.h"
#include <unordered_map>

enum class CartridgeExtType {
  None,
  Mbc1,
  Mbc2,
  Mbc3,
  Mbc5
};

struct CartridgeExtMeta {
  CartridgeExtType type;
  bool hasBattery;
  bool supportsExRam;
};

const std::unordered_map<u8, CartridgeExtMeta> kCartridgeExtMetas {
  { 0x00, { CartridgeExtType::None, false, false } },

  { 0x01, { CartridgeExtType::Mbc1, false, false } },
  { 0x02, { CartridgeExtType::Mbc1, false, true  } },
  { 0x03, { CartridgeExtType::Mbc1, true,  true  } },

  // NOTE: MBC2 contains RAM on-chip, but doesn't actually support cart RAM
  { 0x05, { CartridgeExtType::Mbc2, false, false } },
  { 0x06, { CartridgeExtType::Mbc2, true,  false } },

  { 0x0f, { CartridgeExtType::Mbc3, true,  false } },
  { 0x10, { CartridgeExtType::Mbc3, true,  true  } },
  { 0x11, { CartridgeExtType::Mbc3, false, false } },
  { 0x12, { CartridgeExtType::Mbc3, false, true  } },
  { 0x13, { CartridgeExtType::Mbc3, true,  true  } },

  { 0x19, { CartridgeExtType::Mbc5, false, false } },
  { 0x1a, { CartridgeExtType::Mbc5, false, true  } },
  { 0x1b, { CartridgeExtType::Mbc5, true,  true  } },
  { 0x1c, { CartridgeExtType::Mbc5, false, false } },
  { 0x1d, { CartridgeExtType::Mbc5, false, true  } },
  { 0x1e, { CartridgeExtType::Mbc5, true,  true  } },
};

#endif // SDGBC_CART_EXT_META_H_
