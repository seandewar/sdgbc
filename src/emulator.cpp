#include "emulator.h"

// the minimum amount of time that the emulation thread sleeps for.
// allows the main thread to do work while waiting on the emu thread
constexpr std::chrono::microseconds kMinFrameSleepTime(50);

// the max amount of frames that can be processed at one time without
// sleeping the emulation thread to catch-up with the target framerate
constexpr auto kMaxFrameSkip = 3u;
static_assert(kMaxFrameSkip > 0, "kMaxFrameSkip must be at least 1");

// if the target frame time is kMaxFrameTimeLateness or more behind the
// current time after handling frame skipping, the next frame will be
// rescheduled for the current time
constexpr std::chrono::seconds kMaxFrameTimeLateness(1);

Emulator::Emulator()
    : isPaused_(false), isStarted_(false), limitFramerate_(true) {}

Emulator::~Emulator() {
  StopEmulation();
}

void Emulator::EmulationLoop() {
  using namespace std::chrono;

  // schedule the next frame to happen immediately
  auto nextFrameTime_ = steady_clock::now();

  while (isStarted_) {
    if (isPaused_) {
      // pause and adjust the next frame time to account for the pause
      const auto pauseStartTime = steady_clock::now();
      PauseUntilNotify();

      nextFrameTime_ += steady_clock::now() - pauseStartTime;
    } else {
      // update joypad key states for the frame(s) that we are about to process
      gbc_.GetHardware().joypad.CommitKeyStates();

      if (limitFramerate_) {
        // frame skip until we process enough frames to catch up to our expected
        // frame rate (or until we hit kMaxFrameSkip)
        {
          std::unique_lock<std::mutex> lock(emulationMutex_);

          for (auto i = 0u;
               i < kMaxFrameSkip && steady_clock::now() >= nextFrameTime_;
               ++i) {
            EmulateFrame();
            nextFrameTime_ += duration_cast<steady_clock::duration>(kFrameTime);
          }
        }

        const auto nowTime = steady_clock::now();

        // if the next frame's target time is too far in the past, reschedule it
        if (nowTime - nextFrameTime_ > kMaxFrameTimeLateness) {
          nextFrameTime_ = nowTime;
        }

        // sleep for the remaining frame time or kMinFrameSleepTime if too small
        const auto frameTimeLeft = duration_cast<steady_clock::duration>(
            nextFrameTime_ - nowTime);

        const auto frameSleepDur = std::max(frameTimeLeft,
            duration_cast<steady_clock::duration>(kMinFrameSleepTime));

        std::this_thread::sleep_for(frameSleepDur);
      } else {
        // emulation frame rate unlimited - just render the next frame here and
        // ignore the time that was scheduled for processing the next frame.
        // we wont bother handling frame skip here
        {
          std::unique_lock<std::mutex> lock(emulationMutex_);
          EmulateFrame();
        }

        // sleep for the minimum required time
        std::this_thread::sleep_for(kMinFrameSleepTime);

        // seeing that we ignored the next frame time, we'll reschedule it here
        nextFrameTime_ = steady_clock::now()
                         + duration_cast<steady_clock::duration>(kFrameTime);
      }
    }
  }
}

void Emulator::EmulateFrame() {
  while (normalSpeedFrameCycles_ < kNormalSpeedCyclesPerFrame) {
    // don't scale the amount of cycles left with CPU double speed mode
    const auto cycles = util::RescaleCycles(gbc_.GetHardware().cpu,
                                            gbc_.Update());
    normalSpeedFrameCycles_ += cycles;
  }

  normalSpeedFrameCycles_ -= kNormalSpeedCyclesPerFrame;
}

void Emulator::PauseUntilNotify() {
  std::unique_lock<std::mutex> waitLock(emulationPauseMutex_);

  emulationPauseCondition_.wait(waitLock, [&] () {
    return !isPaused_ || !isStarted_;
  });
}

bool Emulator::StartEmulation() {
  if (!gbc_.GetHardware().cartridge.IsRomLoaded()) {
    return false;
  }

  isStarted_ = true;
  emulationThread_ = std::thread(&Emulator::EmulationLoop, this);
  return true;
}

void Emulator::StopEmulation() {
  // mark emulation as not started and notify the paused condition variable.
  // this is required, as if emulation is paused, the thread will need to be
  // woken up so it knows it has to exit!
  isStarted_ = false;
  emulationPauseCondition_.notify_one();

  // wait for the emulation thread to exit
  if (emulationThread_.joinable()) {
    emulationThread_.join();
  }

  emulationThread_ = std::thread();
}

RomLoadResult Emulator::LoadCartridgeRomFile(const std::string& filePath,
                                             const std::string& fileName) {
  StopEmulation();

  const auto result = gbc_.LoadCartridgeRomFile(filePath, fileName);
  if (result == RomLoadResult::Ok) {
    Reset();
  }

  StartEmulation();
  return result;
}

bool Emulator::ExportCartridgeBatteryExtRam(const std::string& filePath) const {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  return gbc_.GetHardware().cartridge.SaveBatteryExtRam(filePath);
}

bool Emulator::ImportCartridgeBatteryExtRam(const std::string& filePath) {
  StopEmulation();

  const bool success = gbc_.GetHardware().cartridge.LoadBatteryExtRam(filePath);
  if (success) {
    Reset();
  }

  StartEmulation();
  return success;
}

bool Emulator::IsCartridgeRomLoaded() const {
  return gbc_.GetHardware().cartridge.IsRomLoaded();
}

const CartridgeExtMeta& Emulator::GetCartridgeExtMeta() const {
  return gbc_.GetHardware().cartridge.GetExtensionMeta();
}

void Emulator::Reset(bool forceDmgMode) {
  if (gbc_.GetHardware().cartridge.IsRomLoaded()) {
    std::unique_lock<std::mutex> lock(emulationMutex_);

    normalSpeedFrameCycles_ = 0;
    gbc_.Reset(forceDmgMode);
  }
}

std::string Emulator::GetCartridgeRomFileName() const {
  return gbc_.GetHardware().cartridge.GetRomFileName();
}

std::string Emulator::GetCartridgeRomFilePath() const {
  return gbc_.GetHardware().cartridge.GetRomFilePath();
}

void Emulator::SetVideoScanlineSpritesLimiterEnabled(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().ppu.SetScanlineSpritesLimiterEnabled(val);
}

bool Emulator::IsVideoScanlineSpritesLimiterEnabled() const {
  return gbc_.GetHardware().ppu.IsScanlineSpritesLimiterEnabled();
}

void Emulator::SetVideoBgRenderEnabled(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().ppu.SetBgRenderEnabled(val);
}

bool Emulator::IsVideoBgRenderEnabled() const {
  return gbc_.GetHardware().ppu.IsBgRenderEnabled();
}

void Emulator::SetVideoBgWindowRenderEnabled(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().ppu.SetBgWindowRenderEnabled(val);
}

bool Emulator::IsVideoBgWindowRenderEnabled() const {
  return gbc_.GetHardware().ppu.IsBgWindowRenderEnabled();
}

void Emulator::SetVideoSpritesRenderEnabled(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().ppu.SetSpritesRenderEnabled(val);
}

bool Emulator::IsVideoSpritesRenderEnabled() const {
  return gbc_.GetHardware().ppu.IsSpritesRenderEnabled();
}

void Emulator::SetJoypadKeyState(JoypadKey key, bool pressed) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().joypad.SetKeyState(key, pressed);
}

void Emulator::SetJoypadImpossibleInputsAllowed(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().joypad.SetImpossibleInputsAllowed(val);
}

bool Emulator::AreJoypadImpossibleInputsAllowed() const {
  return gbc_.GetHardware().joypad.AreImpossibleInputsAllowed();
}

void Emulator::SetVideoLcd(ILcd* lcd) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().ppu.SetLcd(lcd);
}

void Emulator::SetApuOutput(IApuOutput* audioOut) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().apu.SetApuOutput(audioOut);
}

void Emulator::SetPaused(bool val) {
  isPaused_ = val;

  // if we unpaused, notify the paused condition variable so that the emulation
  // thread can wake back up
  if (!isPaused_) {
    emulationPauseCondition_.notify_one();
  }
}

bool Emulator::IsPaused() const {
  return isPaused_;
}

void Emulator::SetLimitFramerate(bool val) {
  limitFramerate_ = val;
}

bool Emulator::IsLimitingFramerate() const {
  return limitFramerate_;
}

void Emulator::SetApuMuteCh1(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().apu.SetMuteCh1(val);
}

bool Emulator::IsApuCh1Muted() const {
  return gbc_.GetHardware().apu.IsCh1Muted();
}

void Emulator::SetApuMuteCh2(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().apu.SetMuteCh2(val);
}

bool Emulator::IsApuCh2Muted() const {
  return gbc_.GetHardware().apu.IsCh2Muted();
}

void Emulator::SetApuMuteCh3(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().apu.SetMuteCh3(val);
}

bool Emulator::IsApuCh3Muted() const {
  return gbc_.GetHardware().apu.IsCh3Muted();
}

void Emulator::SetApuMuteCh4(bool val) {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  gbc_.GetHardware().apu.SetMuteCh4(val);
}

bool Emulator::IsApuCh4Muted() const {
  return gbc_.GetHardware().apu.IsCh4Muted();
}

bool Emulator::IsStarted() const {
  return isStarted_;
}

bool Emulator::IsInCgbMode() const {
  std::unique_lock<std::mutex> lock(emulationMutex_);
  return gbc_.IsInCgbMode();
}
