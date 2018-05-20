#include "hw/cpu/cpu.h"
#include "util.h"
#include <cassert>

u16 util::To16(u8 hi, u8 lo) {
  return (hi << 8) | lo;
}

u16 util::SetHi8(u16 val, u8 hi) {
  return (hi << 8) | (val & 0xff);
}

u16 util::SetLo8(u16 val, u8 lo) {
  return (val & 0xff00) | lo;
}

u8 util::GetHi8(u16 val) {
  return (val >> 8) & 0xff;
}

u8 util::GetLo8(u16 val) {
  return val & 0xff;
}

u8 util::ReverseBits(u8 val) {
  // divide-and-conquer impl derived from https://stackoverflow.com/a/2602885
  val = (val & 0xf0) >> 4 | (val & 0x0f) << 4;
  val = (val & 0xcc) >> 2 | (val & 0x33) << 2;
  val = (val & 0xaa) >> 1 | (val & 0x55) << 1;
  return val;
}

unsigned int util::RescaleCycles(const Cpu& cpu, unsigned int cycles) {
  // clock cycles should always be a multiple of 2 to allow for rescales.
  // as far as I'm aware, no hardware components work at odd clock cycle
  // intervals in normal-speed mode (min of 4)
  assert(cycles % 2 == 0);
  return cpu.IsInDoubleSpeedMode() ? cycles / 2 : cycles;
}

bool util::ReadBinaryStream(std::istream& is, std::vector<u8>& data,
                            bool resizeToFitData) {
  if ((data.size() <= 0 && !resizeToFitData) || !is) {
    return false;
  }

  // determine the size of the stream and resize the data vector to fit if
  // we're allowed to. resize also avoids a load of reallocs during the read
  is.seekg(0, std::ios::end);
  const auto size = static_cast<std::size_t>(is.tellg());
  if (data.size() < size) {
    if (resizeToFitData) {
      data.resize(size);
    } else {
      return false;
    }
  }

  // read the stream in one (or a few underlying) big chunks. we'll test if
  // the stream's failbit or badbit is set afterwards just in case some IO
  // issue occurred during the read itself
  is.seekg(0, std::ios::beg);
  is.read(reinterpret_cast<char*>(&data[0]), size);
  if (is.fail()) {
    return false;
  }

  // zero fill the remaining contents of the data vector if the file was smaller
  if (size < data.size()) {
    std::fill(data.begin() + size, data.end(), 0);
  }

  return true;
}

bool util::WriteBinaryStream(std::ostream& os, const std::vector<u8>& data) {
  if (data.size() <= 0 || !os) {
    return false;
  }

  os.write(reinterpret_cast<const char*>(&data[0]), data.size());
  return !os.fail();
}
