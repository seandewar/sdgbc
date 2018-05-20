#include "hw/cart/cart_ext_mbc3.h"
#include "hw/cart/cart.h"
#include "util.h"
#include <cassert>

Mbc3::Mbc3(bool timerEnabled) : timerEnabled_(timerEnabled) {}

bool Mbc3::ExtInit() {
  if (cart_->GetNumRomBanks() < 1 || cart_->GetNumRomBanks() > 128 ||
      cart_->GetNum2KBExtRamBanks() > 16) {
    return false;
  }

  // TODO remember to edit code in Cartridge::ParseExtensions() if timer impl
  return MbcBase::ExtInit();
}

void Mbc3::ExtReset() {
  selectedRtc_ = RtcRegister::None;
  rtc_.d = rtc_.s = rtc_.m = rtc_.h = 0; // TODO

  prevLatchVal_ = 0xff;
  isRtcLatched_ = false;

  MbcBase::ExtReset();
}

void Mbc3::ExtRomBank0Write8(u16 loc, u8 val) {
  if (loc < 0x2000) {
    // RAM & RTC enable
    if ((val & 0xf) == 0xa) {
      ramEnabled_ = true;
    } else if (val == 0) {
      ramEnabled_ = false;
    }
  } else {
    // ROM 7-bit bank number select
    romBankNum_ = std::max<u16>(val & 0x7f, 1);
  }
}

void Mbc3::ExtRomBankXWrite8(u16 loc, u8 val) {
  if (loc < 0x2000) {
    if (ramEnabled_) {
      if (val >= 0x8 && val <= 0xc) {
        // RTC select
        selectedRtc_ = static_cast<RtcRegister>(val);
      } else {
        // RAM bank number select
        selectedRtc_ = RtcRegister::None;
        ramBankNum_ = val & 3;
      }
    }
  } else {
    if (prevLatchVal_ == 0 && val == 1) {
      // toggle RTC latch
      latchedRtc_ = rtc_;
      isRtcLatched_ = !isRtcLatched_;
    }
    prevLatchVal_ = val;
  }
}

u8 Mbc3::ReadRtcRegister(RtcRegister rtc) const {
  const auto& activeRtc = isRtcLatched_ ? latchedRtc_ : rtc_;
  switch (rtc) {
    case RtcRegister::S:
      return activeRtc.s;
    case RtcRegister::M:
      return activeRtc.m;
    case RtcRegister::H:
      return activeRtc.h;
    case RtcRegister::Dl:
      return util::GetLo8(activeRtc.d);
    case RtcRegister::Dh:
      return util::GetHi8(activeRtc.d);

    default: assert(!"attempt to read from unmapped MBC3 RTC register!");
      return 0xff;
  }
}

void Mbc3::WriteRtcRegister(RtcRegister rtc, u8 val) {
  auto& activeRtc = isRtcLatched_ ? latchedRtc_ : rtc_;
  switch (rtc) {
    case RtcRegister::S:
      activeRtc.s = val % 60;
      break;
    case RtcRegister::M:
      activeRtc.m = val % 60;
      break;
    case RtcRegister::H:
      activeRtc.h = val % 24;
      break;
    case RtcRegister::Dl:
      activeRtc.d = util::SetLo8(activeRtc.d, val);
      break;
    case RtcRegister::Dh:
      activeRtc.d = util::SetHi8(activeRtc.d, (val & 0xc1) | 0x3e);
      break;
    default: assert(!"attempt to write to unmapped MBC3 RTC register!");
  }
}

u8 Mbc3::ExtRamRead8(u16 loc) const {
  if (selectedRtc_ == RtcRegister::None) {
    return RamExtensionBase::ExtRamRead8(loc);
  } else {
    return ReadRtcRegister(selectedRtc_); // RTC read
  }
}

void Mbc3::ExtRamWrite8(u16 loc, u8 val) {
  if (selectedRtc_ == RtcRegister::None) {
    RamExtensionBase::ExtRamWrite8(loc, val);
  } else {
    WriteRtcRegister(selectedRtc_, val); // RTC write
  }
}
