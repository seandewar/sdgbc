#ifndef SDGBC_CART_EXT_MBC3_H_
#define SDGBC_CART_EXT_MBC3_H_

#include "hw/cart/cart_ext_base.h"

class Mbc3 : public MbcBase {
public:
  explicit Mbc3(bool timerEnabled = false);

  bool ExtInit() override;
  void ExtReset() override;

  void ExtRomBank0Write8(u16 loc, u8 val) override;
  void ExtRomBankXWrite8(u16 loc, u8 val) override;

  void ExtRamWrite8(u16 loc, u8 val) override;
  u8 ExtRamRead8(u16 loc) const override;

private:
  enum class RtcRegister : u8 {
    S  = 0x8,
    M  = 0x9,
    H  = 0xa,
    Dl = 0xb,
    Dh = 0xc,
    None
  };

  struct RtcRegisters {
    u8 s, m, h;
    u16 d;

    RtcRegisters() = default;
    RtcRegisters(const RtcRegisters& other) = default;
    RtcRegisters(RtcRegisters&& other) = delete;
    ~RtcRegisters() = default;

    RtcRegisters& operator=(const RtcRegisters& other) = default;
    RtcRegisters& operator=(RtcRegisters&& other) = delete;
  };

  bool timerEnabled_;

  RtcRegister selectedRtc_;
  RtcRegisters rtc_, latchedRtc_;

  u8 prevLatchVal_;
  bool isRtcLatched_;

  u8 ReadRtcRegister(RtcRegister rtc) const;
  void WriteRtcRegister(RtcRegister rtc, u8 val);
};


#endif // SDGBC_CART_EXT_MBC3_H_
