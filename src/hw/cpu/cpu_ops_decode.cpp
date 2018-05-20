#include "hw/cpu/cpu.h"
#include "util.h"
#include <cassert>

#define NOP(opcode) case opcode: break
#define OP(opcode, expr) case opcode: expr; break

// defines a standard group of 8 opcodes that read from register arguments
#define OP_REG_ARG_READ_GROUP(startOpcode, funcName)       \
    OP(startOpcode    , funcName(reg_.b.Get()));           \
    OP(startOpcode + 1, funcName(reg_.c.Get()));           \
    OP(startOpcode + 2, funcName(reg_.d.Get()));           \
    OP(startOpcode + 3, funcName(reg_.e.Get()));           \
    OP(startOpcode + 4, funcName(reg_.h.Get()));           \
    OP(startOpcode + 5, funcName(reg_.l.Get()));           \
    OP(startOpcode + 6, funcName(IoRead8(reg_.hl.Get()))); \
    OP(startOpcode + 7, funcName(reg_.a.Get()))

// variadic version of OP_REG_ARG_READ_GROUP()
#define OP_REG_ARG_READ_GROUP_VAR(startOpcode, funcName, ...)           \
    OP(startOpcode    , funcName(__VA_ARGS__, reg_.b.Get()));           \
    OP(startOpcode + 1, funcName(__VA_ARGS__, reg_.c.Get()));           \
    OP(startOpcode + 2, funcName(__VA_ARGS__, reg_.d.Get()));           \
    OP(startOpcode + 3, funcName(__VA_ARGS__, reg_.e.Get()));           \
    OP(startOpcode + 4, funcName(__VA_ARGS__, reg_.h.Get()));           \
    OP(startOpcode + 5, funcName(__VA_ARGS__, reg_.l.Get()));           \
    OP(startOpcode + 6, funcName(__VA_ARGS__, IoRead8(reg_.hl.Get()))); \
    OP(startOpcode + 7, funcName(__VA_ARGS__, reg_.a.Get()))

// defines a standard group of 8 opcodes that write to register arguments
#define OP_REG_ARG_WRITE_GROUP(startOpcode, funcName) \
    OP(startOpcode    , funcName(reg_.b));            \
    OP(startOpcode + 1, funcName(reg_.c));            \
    OP(startOpcode + 2, funcName(reg_.d));            \
    OP(startOpcode + 3, funcName(reg_.e));            \
    OP(startOpcode + 4, funcName(reg_.h));            \
    OP(startOpcode + 5, funcName(reg_.l));            \
    OP(startOpcode + 6, funcName(reg_.hl.Get()));     \
    OP(startOpcode + 7, funcName(reg_.a))

bool Cpu::ExecuteOp(u8 op) {
  // standard instructions table
  switch (op) {
    NOP(0x00); // NOP
    OP(0x01, ExecLoad(reg_.bc, IoPcReadNext16())); // LD BC,nn
    OP(0x02, ExecLoad(reg_.bc.Get(), reg_.a.Get())); // LD (BC),A
    OP(0x03, ExecInc16(reg_.bc)); // INC BC
    OP(0x04, ExecInc(reg_.b)); // INC B
    OP(0x05, ExecDec(reg_.b)); // DEC B
    OP(0x06, ExecLoad(reg_.b, IoPcReadNext8())); // LD B,n
    OP(0x07, ExecOpRlca0x07()); // RLCA
    OP(0x08, ExecLoad(IoPcReadNext16(), reg_.sp.Get())); // LD (nn),SP
    OP(0x09, ExecAdd16(reg_.hl, reg_.bc.Get())); // ADD HL,BC
    OP(0x0a, ExecLoad(reg_.a, IoRead8(reg_.bc.Get()))); // LD A,(BC)
    OP(0x0b, ExecDec16(reg_.bc)); // DEC BC
    OP(0x0c, ExecInc(reg_.c)); // INC C
    OP(0x0d, ExecDec(reg_.c)); // DEC C
    OP(0x0e, ExecLoad(reg_.c, IoPcReadNext8())); // LD C,n
    OP(0x0f, ExecOpRrca0x0f()); // RRCA
    OP(0x10, ExecOpStop0x10()); // STOP 0
    OP(0x11, ExecLoad(reg_.de, IoPcReadNext16())); // LD DE,nn
    OP(0x12, ExecLoad(reg_.de.Get(), reg_.a.Get())); // LD (DE),A
    OP(0x13, ExecInc16(reg_.de)); // INC DE
    OP(0x14, ExecInc(reg_.d)); // INC D
    OP(0x15, ExecDec(reg_.d)); // DEC D
    OP(0x16, ExecLoad(reg_.d, IoPcReadNext8())); // LD D,n
    OP(0x17, ExecOpRla0x17()); // RLA
    OP(0x18, ExecOpJr0x18()); // JR n
    OP(0x19, ExecAdd16(reg_.hl, reg_.de.Get())); // ADD HL,DE
    OP(0x1a, ExecLoad(reg_.a, IoRead8(reg_.de.Get()))); // LD A,(DE)
    OP(0x1b, ExecDec16(reg_.de)); // DEC DE
    OP(0x1c, ExecInc(reg_.e)); // INC E
    OP(0x1d, ExecDec(reg_.e)); // DEC E
    OP(0x1e, ExecLoad(reg_.e, IoPcReadNext8())); // LD E,n
    OP(0x1f, ExecOpRra0x1f()); // RRA
    OP(0x20, ExecOpJr0x20()); // JR NZ,n
    OP(0x21, ExecLoad(reg_.hl, IoPcReadNext16())); // LD HL,nn
    OP(0x22, ExecOpLdi0x22()); // LDI (HL),A
    OP(0x23, ExecInc16(reg_.hl)); // INC HL
    OP(0x24, ExecInc(reg_.h)); // INC H
    OP(0x25, ExecDec(reg_.h)); // DEC H
    OP(0x26, ExecLoad(reg_.h, IoPcReadNext8())); // LD H,n
    OP(0x27, ExecOpDaa0x27()); // DAA
    OP(0x28, ExecOpJr0x28()); // JR Z,n
    OP(0x29, ExecAdd16(reg_.hl, reg_.hl.Get())); // ADD HL,HL
    OP(0x2a, ExecOpLdi0x2a()); // LDI A,(HL)
    OP(0x2b, ExecDec16(reg_.hl)); // DEC HL
    OP(0x2c, ExecInc(reg_.l)); // INC L
    OP(0x2d, ExecDec(reg_.l)); // DEC L
    OP(0x2e, ExecLoad(reg_.l, IoPcReadNext8())); // LD L,n
    OP(0x2f, ExecOpCpl0x2f()); // CPL
    OP(0x30, ExecOpJr0x30()); // JR NC,n
    OP(0x31, ExecLoad(reg_.sp, IoPcReadNext16())); // LD SP,nn
    OP(0x32, ExecOpLdd0x32()); // LDD (HL),A
    OP(0x33, ExecInc16(reg_.sp)); // INC SP
    OP(0x34, ExecInc(reg_.hl.Get())); // INC (HL)
    OP(0x35, ExecDec(reg_.hl.Get())); // DEC (HL)
    OP(0x36, ExecLoad(reg_.hl.Get(), IoPcReadNext8())); // LD (HL),n
    OP(0x37, ExecOpScf0x37()); // SCF
    OP(0x38, ExecOpJr0x38()); // JR C,n
    OP(0x39, ExecAdd16(reg_.hl, reg_.sp.Get())); // ADD HL,SP
    OP(0x3a, ExecOpLdd0x3a()); // LDD A,(HL)
    OP(0x3b, ExecDec16(reg_.sp)); // DEC SP
    OP(0x3c, ExecInc(reg_.a)); // INC A
    OP(0x3d, ExecDec(reg_.a)); // DEC A
    OP(0x3e, ExecLoad(reg_.a, IoPcReadNext8())); // LD A,n
    OP(0x3f, ExecOpCcf0x3f()); // CCF

    // LD B,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP_VAR(0x40, ExecLoad, reg_.b);
    // LD C,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP_VAR(0x48, ExecLoad, reg_.c);
    // LD D,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP_VAR(0x50, ExecLoad, reg_.d);
    // LD E,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP_VAR(0x58, ExecLoad, reg_.e);
    // LD H,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP_VAR(0x60, ExecLoad, reg_.h);
    // LD L,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP_VAR(0x68, ExecLoad, reg_.l);
    // LD (HL),v where v = B,C,D,E,H,L
    OP(0x70, ExecLoad(reg_.hl.Get(), reg_.b.Get()));
    OP(0x71, ExecLoad(reg_.hl.Get(), reg_.c.Get()));
    OP(0x72, ExecLoad(reg_.hl.Get(), reg_.d.Get()));
    OP(0x73, ExecLoad(reg_.hl.Get(), reg_.e.Get()));
    OP(0x74, ExecLoad(reg_.hl.Get(), reg_.h.Get()));
    OP(0x75, ExecLoad(reg_.hl.Get(), reg_.l.Get()));

    OP(0x76, ExecOpHalt0x76()); // HALT
    OP(0x77, ExecLoad(reg_.hl.Get(), reg_.a.Get())); // LD (HL),A

    // LD A,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP_VAR(0x78, ExecLoad, reg_.a);
    // ADD A,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x80, ExecAdd);
    // ADC A,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x88, ExecAddWithCarry);
    // SUB A,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x90, ExecSub);
    // SBC A,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x98, ExecSubWithCarry);
    // AND v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0xa0, ExecAnd);
    // XOR v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0xa8, ExecXor);
    // OR v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0xb0, ExecOr);
    // CP v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0xb8, ExecCompare);

    OP(0xc0, ExecReturn(!reg_.f.GetZFlag())); // RET NZ
    OP(0xc1, ExecPop(reg_.bc)); // POP BC
    OP(0xc2, ExecJump(IoPcReadNext16(), !reg_.f.GetZFlag())); // JP NZ,nn
    OP(0xc3, ExecJump(IoPcReadNext16())); // JP nn
    OP(0xc4, ExecCall(IoPcReadNext16(), !reg_.f.GetZFlag())); // CALL NZ,nn
    OP(0xc5, ExecPush(reg_.bc.Get())); // PUSH BC
    OP(0xc6, ExecAdd(IoPcReadNext8())); // ADD A,n
    OP(0xc7, ExecRestart<0x00>()); // RST $00
    OP(0xc8, ExecReturn(reg_.f.GetZFlag())); // RET Z
    OP(0xc9, ExecReturn()); // RET
    OP(0xca, ExecJump(IoPcReadNext16(), reg_.f.GetZFlag())); // JP Z,nn
    OP(0xcb, ExecOpCb0xcb()); // $cb prefix for extended instruction set
    OP(0xcc, ExecCall(IoPcReadNext16(), reg_.f.GetZFlag())); // CALL Z,nn
    OP(0xcd, ExecCall(IoPcReadNext16())); // CALL nn
    OP(0xce, ExecAddWithCarry(IoPcReadNext8())); // ADC A,n
    OP(0xcf, ExecRestart<0x08>()); // RST $08
    OP(0xd0, ExecReturn(!reg_.f.GetCFlag())); // RET NC
    OP(0xd1, ExecPop(reg_.de)); // POP DE
    OP(0xd2, ExecJump(IoPcReadNext16(), !reg_.f.GetCFlag())); // JP NC,nn
    OP(0xd4, ExecCall(IoPcReadNext16(), !reg_.f.GetCFlag())); // CALL NC,nn
    OP(0xd5, ExecPush(reg_.de.Get())); // PUSH DE
    OP(0xd6, ExecSub(IoPcReadNext8())); // SUB A,n
    OP(0xd7, ExecRestart<0x10>()); // RST $10
    OP(0xd8, ExecReturn(reg_.f.GetCFlag())); // RET C
    OP(0xd9, ExecOpReti0xd9()); // RETI
    OP(0xda, ExecJump(IoPcReadNext16(), reg_.f.GetCFlag())); // JP C,nn
    OP(0xdc, ExecCall(IoPcReadNext16(), reg_.f.GetCFlag())); // CALL C,nn
    OP(0xde, ExecSubWithCarry(IoPcReadNext8())); // SBC A,n
    OP(0xdf, ExecRestart<0x18>()); // RST $18
    OP(0xe0, ExecLoad(0xff00 + IoPcReadNext8(), reg_.a.Get())); // LD (n),A
    OP(0xe1, ExecPop(reg_.hl)); // POP HL
    OP(0xe2, ExecLoad(0xff00 + reg_.c.Get(), reg_.a.Get())); // LD (C),A
    OP(0xe5, ExecPush(reg_.hl.Get())); // PUSH HL
    OP(0xe6, ExecAnd(IoPcReadNext8())); // AND n
    OP(0xe7, ExecRestart<0x20>()); // RST $20
    OP(0xe8, ExecOpAdd0xe8()); // ADD SP,n
    OP(0xe9, ExecOpJp0xe9()); // JP (HL)
    OP(0xea, ExecLoad(IoPcReadNext16(), reg_.a.Get())); // LD (nn),A
    OP(0xee, ExecXor(IoPcReadNext8())); // XOR n
    OP(0xef, ExecRestart<0x28>()); // RST $28
    OP(0xf0, ExecLoad(reg_.a, IoRead8(0xff00 + IoPcReadNext8()))); // LD A,(n)
    OP(0xf1, ExecPop(reg_.af)); // POP AF
    OP(0xf2, ExecLoad(reg_.a, IoRead8(0xff00 + reg_.c.Get()))); // LD A,(C)
    OP(0xf3, ExecOpDi0xf3()); // DI
    OP(0xf5, ExecPush(reg_.af.Get())); // PUSH AF
    OP(0xf6, ExecOr(IoPcReadNext8())); // OR n
    OP(0xf7, ExecRestart<0x30>()); // RST $30
    OP(0xf8, ExecOpLdhl0xf8()); // LDHL SP,n
    OP(0xf9, ExecOpLd0xf9()); // LD SP,HL
    OP(0xfa, ExecLoad(reg_.a, IoRead8(IoPcReadNext16()))); // LD A,(nn)
    OP(0xfb, ExecOpEi0xfb()); // EI
    OP(0xfe, ExecCompare(IoPcReadNext8())); // CP n
    OP(0xff, ExecRestart<0x38>()); // RST $38

    // unknown op
    default:
      return false;
  }

  return true;
}

void Cpu::ExecuteExOp(u8 exOp) {
  // extended instructions ($cb prefix) table
  switch (exOp) {
    // RLC v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x00, ExecRotLeft);
    // RRC v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x08, ExecRotRight);
    // RL v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x10, ExecRotLeftThroughCarry);
    // RR v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x18, ExecRotRightThroughCarry);
    // SLA v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x20, ExecShiftLeft);
    // SRA v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x28, ExecShiftRightSigned);
    // SWAP v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x30, ExecSwap);
    // SRL v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x38, ExecShiftRight);
    // BIT 0,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x40, ExecTestBit<0>);
    // BIT 1,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x48, ExecTestBit<1>);
    // BIT 2,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x50, ExecTestBit<2>);
    // BIT 3,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x58, ExecTestBit<3>);
    // BIT 4,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x60, ExecTestBit<4>);
    // BIT 5,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x68, ExecTestBit<5>);
    // BIT 6,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x70, ExecTestBit<6>);
    // BIT 7,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_READ_GROUP(0x78, ExecTestBit<7>);
    // RES 0,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x80, ExecResetBit<0>);
    // RES 1,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x88, ExecResetBit<1>);
    // RES 2,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x90, ExecResetBit<2>);
    // RES 3,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0x98, ExecResetBit<3>);
    // RES 4,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xa0, ExecResetBit<4>);
    // RES 5,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xa8, ExecResetBit<5>);
    // RES 6,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xb0, ExecResetBit<6>);
    // RES 7,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xb8, ExecResetBit<7>);
    // SET 0,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xc0, ExecSetBit<0>);
    // SET 1,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xc8, ExecSetBit<1>);
    // SET 2,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xd0, ExecSetBit<2>);
    // SET 3,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xd8, ExecSetBit<3>);
    // SET 4,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xe0, ExecSetBit<4>);
    // SET 5,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xe8, ExecSetBit<5>);
    // SET 6,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xf0, ExecSetBit<6>);
    // SET 7,v where v = B,C,D,E,H,L,(HL),A
    OP_REG_ARG_WRITE_GROUP(0xf8, ExecSetBit<7>);

    // unknown ex op - this shouldn't happen as all ex ops should be mapped
    default: assert(!"unknown ex opcode - all ex ops should be handled!");
  }
}
