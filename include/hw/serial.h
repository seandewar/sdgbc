#ifndef SDGBC_SERIAL_H_
#define SDGBC_SERIAL_H_

#include "types.h"

class ISerialOutput {
public:
  virtual ~ISerialOutput() = default;

  virtual void SerialReset() = 0;
  virtual void SerialWriteBit(bool bitSet) = 0;
  virtual void SerialOnByteWritten() = 0;
};

class Cpu;

class Serial {
public:
  explicit Serial(Cpu& cpu);

  void Reset(bool cgbMode);
  void Update(unsigned int cycles);

  void SetSerialOutput(ISerialOutput* dataOut);

  void SetSb(u8 val);
  u8 GetSb() const;

  void SetSc(u8 val);
  u8 GetSc() const;

  bool IsInCgbMode() const;

private:
  Cpu& cpu_;

  ISerialOutput* dataOut_;

  u8 sb_, sc_;
  unsigned int nextBitTransferCycles_;
  u8 transferNextBitIdx_;

  bool cgbMode_;
};

#endif // SDGBC_SERIAL_H_
