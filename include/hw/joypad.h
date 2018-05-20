#ifndef SDGBC_JOYPAD_H_
#define SDGBC_JOYPAD_H_

#include "types.h"

enum class JoypadKey : u8 {
  A      = 0x01,
  B      = 0x02,
  Select = 0x04,
  Start  = 0x08,
  Right  = 0x10,
  Left   = 0x20,
  Up     = 0x40,
  Down   = 0x80
};

class Cpu;

class Joypad {
public:
  explicit Joypad(Cpu& cpu);

  void Reset();

  void CommitKeyStates();
  void SetKeyState(JoypadKey key, bool pressed);

  void SetJoyp(u8 val);
  u8 GetJoyp() const;

  void SetImpossibleInputsAllowed(bool val);
  bool AreImpossibleInputsAllowed() const;

  bool WasSelectedKeyPressed() const;

private:
  Cpu& cpu_;

  u8 keyStates_, nextKeyStates_;
  bool selectButtonKeys_, selectDirectionKeys_;
  bool wasSelectedKeyPressed_;

  bool leftRightOrUpDownAllowed_;
};

#endif // SDGBC_JOYPAD_H_
