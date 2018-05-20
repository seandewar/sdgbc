#include "audio/sfml_apu_out.h"

SfmlApuSoundStream::SfmlApuSoundStream()
    : isStreaming_(false),
      sampleFrontBuffer_(&sampleBuffers_[0]),
      sampleBackBuffer_(&sampleBuffers_[1]) {
  // 2 channels for left and right speakers @ our APU's downsampled sample rate
  initialize(2, kApuOutputSampleRateHz);
}

SfmlApuSoundStream::~SfmlApuSoundStream() {
  StopStreaming();
}

void SfmlApuSoundStream::StartStreaming() {
  {
    std::unique_lock<std::mutex> lock(sampleBackBufferMutex_);
    samplesNextIdx_ = 0;
  }

  isStreaming_ = true;
  play();
}

void SfmlApuSoundStream::StopStreaming() {
  isStreaming_ = false;
  streamWaitCondition_.notify_one();
}

bool SfmlApuSoundStream::IsStreaming() const {
  return isStreaming_;
}

bool SfmlApuSoundStream::AudioIsMuted() const {
  return !isStreaming_;
}

void SfmlApuSoundStream::AudioBufferSamples(i16 leftSample, i16 rightSample) {
  if (samplesNextIdx_ < sampleBackBuffer_->size()) {
    // locking here has the potential that samplesNextIdx_ is set to 0 by the
    // onGetData() thread, but this shouldn't cause any issues (it'll mean that
    // these samples will get queued for the next audio driver copy instead)
    std::unique_lock<std::mutex> lock(sampleBackBufferMutex_);

    (*sampleBackBuffer_)[samplesNextIdx_++] = leftSample;
    (*sampleBackBuffer_)[samplesNextIdx_++] = rightSample;
  }

  // wake up onGetData() if it is waiting for more samples if we've now
  // collected enough
  if (samplesNextIdx_ >= kMinStreamedSamples) {
    streamWaitCondition_.notify_one();
  }
}

bool SfmlApuSoundStream::onGetData(Chunk& data) {
  // wait until we have enough samples first (at least kMinStreamedSamples)
  {
    std::unique_lock<std::mutex> waitLock(streamWaitMutex_);

    streamWaitCondition_.wait(waitLock, [&] () {
      return samplesNextIdx_ >= kMinStreamedSamples || !isStreaming_;
    });
  }

  if (!isStreaming_) {
    return false; // returning false stops the stream
  }

  {
    std::unique_lock<std::mutex> lock(sampleBackBufferMutex_);

    std::swap(sampleFrontBuffer_, sampleBackBuffer_);
    data.sampleCount = samplesNextIdx_;
    samplesNextIdx_ = 0;
  }

  // the buffer will be copied to the audio driver at the end of this function
  data.samples = &(*sampleFrontBuffer_)[0];
  return true;
}

void SfmlApuSoundStream::onSeek(sf::Time) {} // don't support seeks
