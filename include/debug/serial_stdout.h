#ifndef SDGBC_SERIAL_STDOUT_H_
#define SDGBC_SERIAL_STDOUT_H_

#include "hw/serial.h"
#include <iostream>

// TODO: this could be generalized into using an ostream, but as we're not
// implementing serial properly (due to requirements), this is fine for now
class SerialStdout : public ISerialOutput {
public:
  void SerialReset() override;

  void SerialWriteBit(bool bitSet) override;
  void SerialOnByteWritten() override;

private:
  u8 outByte_;
};

#endif // SDGBC_SERIAL_STDOUT_H_
