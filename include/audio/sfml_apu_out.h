#ifndef SDGBC_SFML_APU_OUT_H_
#define SDGBC_SFML_APU_OUT_H_

#include "hw/apu/apu.h"
#include "types.h"
#include <SFML/Audio.hpp>
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>

// the maximum amount of samples that can be buffered at any one time.
// this doesn't include the amount of samples that are currently buffered by the
// audio driver for being played, just the samples that haven't been sent yet
constexpr std::size_t kMaxSampleBufferSize = 4096;

// the minimum amount of samples required to update the audio stream.
// onGetData() will block until this many samples are acquired before sending
// data to the audio driver
constexpr std::size_t kMinStreamedSamples = 2048;

using SampleBuffer = std::array<i16, kMaxSampleBufferSize>;

// NOTE: we privately inherit from sf::SoundStream to disallow external code
// from calling play(), stop() etc. manually. This is because our stream blocks
// until it receives enough data to send to the audio driver. Unless we unblock
// first, these methods will never return - StopStreaming() and StartStreaming()
// handles these cases properly
class SfmlApuSoundStream : private sf::SoundStream, public IApuOutput {
  static_assert(kMaxSampleBufferSize % 2 == 0,
                "kMaxSampleBufferSize must be a multiple of 2 (2 channels)");

public:
  SfmlApuSoundStream();
  ~SfmlApuSoundStream();

  void StartStreaming();
  void StopStreaming();

  void AudioBufferSamples(i16 leftSample, i16 rightSample) override;
  bool AudioIsMuted() const override;

  bool IsStreaming() const;

private:
  // we use double buffering for samples.
  // this is because the sound thread will copy to the audio driver after
  // onGetData() returns, which means unlocked sample writes after onGetData()
  // can potentially affect affect the copy that gets sent to the audio device
  std::array<SampleBuffer, 2> sampleBuffers_;
  SampleBuffer* sampleFrontBuffer_;
  SampleBuffer* sampleBackBuffer_;
  // NOTE: only the back buffer can be accessed by the sound and emulation
  // thread at the same time. this is possible during the swap, which is started
  // by the sound thread in onGetData()
  mutable std::mutex sampleBackBufferMutex_;
  std::atomic<std::size_t> samplesNextIdx_;

  mutable std::mutex streamWaitMutex_;
  std::condition_variable streamWaitCondition_;
  std::atomic<bool> isStreaming_;

  bool onGetData(Chunk& data) override;
  void onSeek(sf::Time timeOffset) override;
};

#endif // SDGBC_SFML_APU_OUT_H_
