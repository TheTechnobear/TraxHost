#pragma once


static constexpr unsigned int kAudioBufSize = 128;
static constexpr unsigned int kAudioSampleRate = 48000;

#ifdef __APPLE__
static constexpr unsigned int kAudioInCh = 8;
static constexpr unsigned int kAudioOutCh = 2;
static constexpr int kInChMap[kAudioInCh] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static constexpr int kOutChMap[kAudioOutCh] = { 0, 1 };
static constexpr float inGain = 1.0f;
static constexpr float outGain = 1.0f;
static constexpr float inOffset = 0.f;
static constexpr float outOffset = 0.f;
#elif defined(TARGET_SSP)
static constexpr unsigned int kAudioInCh = 16;
static constexpr unsigned int kAudioOutCh = 8;
static constexpr int kInChMap[kAudioInCh] = { 11, 10, 9, 8, 15, 14, 13, 12, 3, 2, 1, 0, 7, 6, 5, 4 };
static constexpr int kOutChMap[kAudioOutCh] = { 3, 2, 1, 0, 7, 6, 5, 4 };
static constexpr float inGain = 0.2f / 0.18795f;
static constexpr float outGain = 5.0f / 5.248f;
static constexpr float inOffset = 0.02300f;
static constexpr float outOffset = 0.f;
#else
static constexpr unsigned int kAudioInCh = 8;
static constexpr unsigned int kAudioOutCh = 2;
static constexpr int kInChMap[kAudioInCh] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static constexpr int kOutChMap[kAudioOutCh] = { 0, 1 };
static constexpr float inGain = -1.0f;
static constexpr float outGain = -1.0f;
// static constexpr float outGain = -5.0f / 2.36;
static constexpr float inOffset = 0.f;
static constexpr float outOffset = 0.f;

#endif

#include <RtAudio.h>

namespace TraxHost {

class Module;

struct AudioApi {
    RtAudio rtAudio_;
    unsigned audioInDeviceId = 0;
    unsigned audioOutDeviceId = 0;
    unsigned sampleRate = kAudioSampleRate;
    unsigned bufSize = kAudioBufSize;
    unsigned inCh = kAudioInCh;
    unsigned outCh = kAudioOutCh;
    Module *module = nullptr;
};

int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/, double streamTime,
                  RtAudioStreamStatus status, void *data);


bool initAudio(AudioApi &api, TraxHost::Module &module);

bool startAudio(AudioApi &api);

void stopAudio(AudioApi &api);

}  // namespace TraxHost