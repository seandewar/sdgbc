#include "debug/serial_stdout.h"
#include <iostream>

void SerialStdout::SerialReset() {
  outByte_ = 0;
}

void SerialStdout::SerialWriteBit(bool bitSet) {
  outByte_ = ((outByte_ << 1) | (bitSet ? 1 : 0)) & 0xff;
}

void SerialStdout::SerialOnByteWritten() {
  std::cout << outByte_;
}
