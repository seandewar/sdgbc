#ifndef SDGBC_MMU_H_
#define SDGBC_MMU_H_

#include "hw/memory.h"

enum IoRegisterId : u8 {
  kIoRegisterIdJoyp  = 0x00,

  kIoRegisterIdSb    = 0x01,
  kIoRegisterIdSc    = 0x02,

  kIoRegisterIdDiv   = 0x04,
  kIoRegisterIdTima  = 0x05,
  kIoRegisterIdTma   = 0x06,
  kIoRegisterIdTac   = 0x07,

  kIoRegisterIdIntf  = 0x0f,

  kIoRegisterIdNr10  = 0x10,
  kIoRegisterIdNr11  = 0x11,
  kIoRegisterIdNr12  = 0x12,
  kIoRegisterIdNr13  = 0x13,
  kIoRegisterIdNr14  = 0x14,
  kIoRegisterIdNr21  = 0x16,
  kIoRegisterIdNr22  = 0x17,
  kIoRegisterIdNr23  = 0x18,
  kIoRegisterIdNr24  = 0x19,
  kIoRegisterIdNr30  = 0x1a,
  kIoRegisterIdNr31  = 0x1b,
  kIoRegisterIdNr32  = 0x1c,
  kIoRegisterIdNr33  = 0x1d,
  kIoRegisterIdNr34  = 0x1e,
  kIoRegisterIdNr41  = 0x20,
  kIoRegisterIdNr42  = 0x21,
  kIoRegisterIdNr43  = 0x22,
  kIoRegisterIdNr44  = 0x23,
  kIoRegisterIdNr50  = 0x24,
  kIoRegisterIdNr51  = 0x25,
  kIoRegisterIdNr52  = 0x26,

  kIoRegisterIdLcdc  = 0x40,
  kIoRegisterIdStat  = 0x41,
  kIoRegisterIdScy   = 0x42,
  kIoRegisterIdScx   = 0x43,
  kIoRegisterIdLy    = 0x44,
  kIoRegisterIdLyc   = 0x45,
  kIoRegisterIdDma   = 0x46,
  kIoRegisterIdBgp   = 0x47,
  kIoRegisterIdObp0  = 0x48,
  kIoRegisterIdObp1  = 0x49,
  kIoRegisterIdWy    = 0x4a,
  kIoRegisterIdWx    = 0x4b,

  kIoRegisterIdKey1  = 0x4d,

  kIoRegisterIdVbk   = 0x4f,
  kIoRegisterIdHdma1 = 0x51,
  kIoRegisterIdHdma2 = 0x52,
  kIoRegisterIdHdma3 = 0x53,
  kIoRegisterIdHdma4 = 0x54,
  kIoRegisterIdHdma5 = 0x55,
  kIoRegisterIdBcps  = 0x68,
  kIoRegisterIdBcpd  = 0x69,
  kIoRegisterIdOcps  = 0x6a,
  kIoRegisterIdOcpd  = 0x6b,

  kIoRegisterIdSvbk  = 0x70,

  kIoRegisterIdInte  = 0xff
};

struct GbcHardware;

class Mmu {
public:
  explicit Mmu(GbcHardware& hw);

  void Reset(bool cgbMode);

  void Write8(u16 loc, u8 val);
  u8 Read8(u16 loc) const;

  void WriteIoRegister(u8 regId, u8 val);
  u8 ReadIoRegister(u8 regId) const;

  bool IsInCgbMode() const;

private:
  GbcHardware& hw_;
  bool cgbMode_;

  // WRAM bank switch register
  u8 svbk_;

  u8 GetWramBankIndex() const;
};

#endif // SDGBC_MMU_H_
