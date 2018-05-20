#ifndef SDGBC_EMULATOR_H_
#define SDGBC_EMULATOR_H_

#include "hw/gbc.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

// VBlank happens at approx. 59.7275 Hz
using FrameDurationMillis = std::chrono::duration<int_least64_t,
                                                  std::ratio<10000, 597275>>;
constexpr FrameDurationMillis kFrameTime(1);

// VBlank takes 70224 clock cycles in normal speed mode.
// 4.194304 MHz div 59.7275 = approx 70224 clock cycles
constexpr auto kNormalSpeedCyclesPerFrame = 70224u;

// system is clocked at 4.194304 MHz in normal speed mode
constexpr auto kNormalSpeedClockRateHz = 4194304u;

class Emulator {
public:
  Emulator();
  explicit Emulator(const Emulator& other) = delete;
  explicit Emulator(Emulator&& other) = delete;
  ~Emulator();

  Emulator& operator=(const Emulator& other) = delete;
  Emulator& operator=(Emulator&& other) = delete;

  RomLoadResult LoadCartridgeRomFile(const std::string& filePath,
                                     const std::string& fileName = {});

  bool ExportCartridgeBatteryExtRam(const std::string& filePath) const;
  bool ImportCartridgeBatteryExtRam(const std::string& filePath);

  void Reset(bool forceDmgMode = false);

  void SetVideoLcd(ILcd* lcd);
  void SetApuOutput(IApuOutput* audioOut);

  void SetJoypadKeyState(JoypadKey key, bool pressed);

  void SetJoypadImpossibleInputsAllowed(bool val);
  bool AreJoypadImpossibleInputsAllowed() const;

  void SetPaused(bool val);
  bool IsPaused() const;

  void SetLimitFramerate(bool val);
  bool IsLimitingFramerate() const;

  bool IsStarted() const;

  void SetApuMuteCh1(bool val);
  bool IsApuCh1Muted() const;

  void SetApuMuteCh2(bool val);
  bool IsApuCh2Muted() const;

  void SetApuMuteCh3(bool val);
  bool IsApuCh3Muted() const;

  void SetApuMuteCh4(bool val);
  bool IsApuCh4Muted() const;

  void SetVideoScanlineSpritesLimiterEnabled(bool val);
  bool IsVideoScanlineSpritesLimiterEnabled() const;

  void SetVideoBgRenderEnabled(bool val);
  bool IsVideoBgRenderEnabled() const;

  void SetVideoBgWindowRenderEnabled(bool val);
  bool IsVideoBgWindowRenderEnabled() const;

  void SetVideoSpritesRenderEnabled(bool val);
  bool IsVideoSpritesRenderEnabled() const;

  bool IsCartridgeRomLoaded() const;
  const CartridgeExtMeta& GetCartridgeExtMeta() const;
  std::string GetCartridgeRomFileName() const;
  std::string GetCartridgeRomFilePath() const;

  bool IsInCgbMode() const;

private:
  Gbc gbc_;

  std::atomic<bool> isPaused_, isStarted_, limitFramerate_;
  unsigned int normalSpeedFrameCycles_;

  std::thread emulationThread_;
  // NOTE: access of some emulated hw properties (such as cartridge ROM info)
  // does not require locking, as these properties are only ever mutated when
  // emulation is not running, or aren't mutated by the emulation thread at
  // all (such as APU channel mute status)
  mutable std::mutex emulationMutex_;

  std::condition_variable emulationPauseCondition_;
  mutable std::mutex emulationPauseMutex_;

  bool StartEmulation();
  void StopEmulation();

  void EmulationLoop();
  void EmulateFrame();
  void PauseUntilNotify();
};

#endif // SDGBC_EMULATOR_H_
