#ifndef SDGBC_CPU_REG_BASE_H_
#define SDGBC_CPU_REG_BASE_H_

#include "types.h"
#include "util.h"
#include <type_traits>

template <typename C, typename T>
class CpuRegister {
  static_assert(std::is_integral<T>::value, "T must be an integral type");

public:
  void Set(T val);
  T Get() const;

  // prefix inc/dec operators
  C& operator++();
  C& operator--();

  // postfix inc/dec operators
  T operator++(int);
  T operator--(int);

private:
  C& AsC();
  const C& AsC() const;
};

template <typename T>
class CpuBasicRegister : public CpuRegister<CpuBasicRegister<T>, T> {
  static_assert(std::is_integral<T>::value, "T must be an integral type");

public:
  explicit CpuBasicRegister(T val = 0);

  void SetImpl(T val);
  T GetImpl() const;

private:
  T val_;
};

using CpuRegisterBasic8  = CpuBasicRegister<u8>;
using CpuRegisterBasic16 = CpuBasicRegister<u16>;

template <typename H, typename L>
class CpuRegister8Pair {
  static_assert(std::is_base_of<CpuRegister<H, u8>, H>::value,
                "H must be derived from type CpuRegister<H, u8>");
  static_assert(std::is_base_of<CpuRegister<L, u8>, L>::value,
                "L must be derived from type CpuRegister<L, u8>");

public:
  explicit CpuRegister8Pair(CpuRegister<H, u8>& hi, CpuRegister<L, u8>& lo);

  void Set(u16 val);
  u16 Get() const;

  // prefix inc/dec operators
  CpuRegister8Pair<H, L>& operator++();
  CpuRegister8Pair<H, L>& operator--();

  // postfix inc/dec operators
  u16 operator++(int);
  u16 operator--(int);

  CpuRegister<H, u8>& GetHi();
  const CpuRegister<H, u8>& GetHi() const;

  CpuRegister<L, u8>& GetLo();
  const CpuRegister<L, u8>& GetLo() const;

private:
  CpuRegister<H, u8>& hi_;
  CpuRegister<L, u8>& lo_;
};

using CpuRegisterBasic8Pair = CpuRegister8Pair<CpuRegisterBasic8,
                                               CpuRegisterBasic8>;

#include "hw/cpu/cpu_reg_base_inl.h"

#endif // SDGBC_CPU_REG_BASE_H_
