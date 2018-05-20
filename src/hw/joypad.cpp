#include "hw/cpu/cpu.h"
#include "hw/joypad.h"

Joypad::Joypad(Cpu& cpu) : cpu_(cpu), leftRightOrUpDownAllowed_(false) {}

void Joypad::Reset() {
  keyStates_ = nextKeyStates_ = 0;
  selectButtonKeys_ = selectDirectionKeys_ = true;

  wasSelectedKeyPressed_ = false;
}

void Joypad::CommitKeyStates() {
  const u8 nowPressedKeys = nextKeyStates_ & ~keyStates_;

  wasSelectedKeyPressed_ = ((nowPressedKeys & 0x0f) && selectButtonKeys_) ||
                           ((nowPressedKeys & 0xf0) && selectDirectionKeys_);
  if (wasSelectedKeyPressed_) {
    // a selected key that was prev unpressed was since pressed. trigger int $60
    cpu_.IntfRequest(kCpuInterrupt0x60);
  }

  keyStates_ = nextKeyStates_;
}

void Joypad::SetImpossibleInputsAllowed(bool val) {
  leftRightOrUpDownAllowed_ = val;
}

bool Joypad::AreImpossibleInputsAllowed() const {
  return leftRightOrUpDownAllowed_;
}

bool Joypad::WasSelectedKeyPressed() const {
  return wasSelectedKeyPressed_;
}

void Joypad::SetKeyState(JoypadKey key, bool pressed) {
  if (pressed) {
    if (!leftRightOrUpDownAllowed_) {
      // if we don't allow simultaneous presses of up+down or left+right (which
      // the real hardware doesn't also doesn't allow), unpress the opposite
      // direction key
      switch (key) {
        case JoypadKey::Up:
          SetKeyState(JoypadKey::Down, false);
          break;
        case JoypadKey::Down:
          SetKeyState(JoypadKey::Up, false);
          break;
        case JoypadKey::Left:
          SetKeyState(JoypadKey::Right, false);
          break;
        case JoypadKey::Right:
          SetKeyState(JoypadKey::Left, false);
          break;
      }
    }

    nextKeyStates_ |= static_cast<u8>(key);
  } else {
    nextKeyStates_ &= ~static_cast<u8>(key);
  }
}

void Joypad::SetJoyp(u8 val) {
  selectButtonKeys_ = (val & 0x20) == 0;
  selectDirectionKeys_ = (val & 0x10) == 0;
}

u8 Joypad::GetJoyp() const {
  // set all button input lines to high initially, and set bit 5 and 4 to
  // indicate whether the button/direction keys are selected.
  // bits 7 & 6 are always set (and are unused)
  u8 joyp = 0xcf | (selectButtonKeys_ ? 0x20 : 0)
                 | (selectDirectionKeys_ ? 0x10 : 0);

  // joypad register works on inverses as it doesn't use an inverter
  // (1 = unpressed, 0 = pressed), so complement the state values we have first
  if (selectButtonKeys_) {
    joyp = (joyp & 0xf0) | (~keyStates_ & 0xf);
  }
  if (selectDirectionKeys_) {
    joyp = (joyp & 0xf0) | ((~keyStates_ >> 4) & 0xf);
  }

  return joyp;
}
