#ifndef SDGBC_CPU_REG_BASE_INL_H_
#define SDGBC_CPU_REG_BASE_INL_H_

template <typename C, typename T>
void CpuRegister<C, T>::Set(T val) {
  AsC().SetImpl(val);
}

template <typename C, typename T>
T CpuRegister<C, T>::Get() const {
  return AsC().GetImpl();
}

// prefix inc/dec operators
template <typename C, typename T>
C& CpuRegister<C, T>::operator++() {
  Set(Get() + 1);
  return AsC();
}

template <typename C, typename T>
C& CpuRegister<C, T>::operator--() {
  Set(Get() - 1);
  return AsC();
}

// postfix inc/dec operators
template <typename C, typename T>
T CpuRegister<C, T>::operator++(int) {
  const T beforeVal = Get();
  Set(Get() + 1);

  return beforeVal;
}

template <typename C, typename T>
T CpuRegister<C, T>::operator--(int) {
  const T beforeVal = Get();
  Set(Get() - 1);

  return beforeVal;
}

template <typename C, typename T>
C& CpuRegister<C, T>::AsC() {
  return *static_cast<C*>(this);
}

template <typename C, typename T>
const C& CpuRegister<C, T>::AsC() const {
  return *static_cast<const C*>(this);
}

template <typename T>
CpuBasicRegister<T>::CpuBasicRegister(T val) : val_(val) {}

template <typename T>
void CpuBasicRegister<T>::SetImpl(T val) {
  val_ = val;
}

template <typename T>
T CpuBasicRegister<T>::GetImpl() const {
  return val_;
}

template <typename H, typename L>
CpuRegister8Pair<H, L>::CpuRegister8Pair(CpuRegister<H, u8>& hi,
                                         CpuRegister<L, u8>& lo)
    : hi_(hi), lo_(lo) {}

template <typename H, typename L>
void CpuRegister8Pair<H, L>::Set(u16 val) {
  hi_.Set(util::GetHi8(val));
  lo_.Set(util::GetLo8(val));
}

template <typename H, typename L>
u16 CpuRegister8Pair<H, L>::Get() const {
  return util::To16(hi_.Get(), lo_.Get());
}

// prefix inc/dec operators
template <typename H, typename L>
CpuRegister8Pair<H, L>& CpuRegister8Pair<H, L>::operator++() {
  Set(Get() + 1);
  return *this;
}

template <typename H, typename L>
CpuRegister8Pair<H, L>& CpuRegister8Pair<H, L>::operator--() {
  Set(Get() - 1);
  return *this;
}

// postfix inc/dec operators
template <typename H, typename L>
u16 CpuRegister8Pair<H, L>::operator++(int) {
  const u16 beforeVal = Get();
  Set(Get() + 1);

  return beforeVal;
}

template <typename H, typename L>
u16 CpuRegister8Pair<H, L>::operator--(int) {
  const u16 beforeVal = Get();
  Set(Get() - 1);

  return beforeVal;
}

template <typename H, typename L>
CpuRegister<H, u8>& CpuRegister8Pair<H, L>::GetHi() {
  return hi_;
}

template <typename H, typename L>
const CpuRegister<H, u8>& CpuRegister8Pair<H, L>::GetHi() const {
  return hi_;
}

template <typename H, typename L>
CpuRegister<L, u8>& CpuRegister8Pair<H, L>::GetLo() {
  return lo_;
}

template <typename H, typename L>
const CpuRegister<L, u8>& CpuRegister8Pair<H, L>::GetLo() const {
  return lo_;
}

#endif // SDGBC_CPU_REG_BASE_INL_H_
