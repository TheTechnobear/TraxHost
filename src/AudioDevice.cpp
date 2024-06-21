#include "AudioDevice.h"
#include "Module.h"
#include "Log.h"

namespace TraxHost {


static bool firstCall = true;

int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/, double streamTime,
                  RtAudioStreamStatus status, void *data) {
#ifndef __APPLE__
    if (firstCall) {
        firstCall = false;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(1, &cpuset);
        int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            TraxHost::error("TraxHost AudioThread,  Error calling pthread_setaffinity_np: " + std::to_string(rc));
        } else {
            TraxHost::log("TraxHost , Set affinity for audio thread");
        }
    }
#endif

    if (status) TraxHost::error(std::string("Stream over/underflow detected."));

    AudioApi *api = (AudioApi *)data;
    float *inBuf = (float *)inputBuffer;
    float *outBuf = (float *)outputBuffer;

    for (int i = 0, n = api->inCh * api->bufSize; i < n; i++) inBuf[i] = inBuf[i] * inGain + inOffset;
    api->module->audioCallback(inBuf, api->inCh, outBuf, api->outCh, api->bufSize);
    for (int i = 0, n = api->outCh * api->bufSize; i < n; i++) outBuf[i] = outBuf[i] * outGain + outOffset;

    return 0;
}


bool initAudio(AudioApi &api, TraxHost::Module &module) {
    RtAudio &audioApi = api.rtAudio_;

    audioApi.showWarnings(true);
    api.module = &module;

    std::vector<unsigned int> ids = audioApi.getDeviceIds();
    RtAudio::DeviceInfo info;

#ifdef __APPLE__
    static constexpr const char *audioDevNamePrefix = "XMX";
#elif defined(TARGET_SSP)
    static constexpr const char *audioDevNamePrefix = "ak4458";
#else
    static constexpr const char *audioDevNamePrefix = "rockchip";
#endif

    unsigned xmxAudioInDeviceId = 0;
    unsigned xmxAudioOutDeviceId = 0;
    for (unsigned int n = 0; n < ids.size(); n++) {
        info = audioApi.getDeviceInfo(ids[n]);
        if (info.name.find(audioDevNamePrefix) != std::string::npos) {
            TraxHost::log(std::string("found audio device: ") + info.name + std::string(", id: ") +
                          std::to_string(ids[n]));
            TraxHost::log(std::string("out: ") + std::to_string(info.outputChannels) + std::string(", in: ") +
                          std::to_string(info.inputChannels));
            if (info.outputChannels >= 2) { xmxAudioOutDeviceId = ids[n]; }
            if (info.inputChannels >= 8) { xmxAudioInDeviceId = ids[n]; }
            if (xmxAudioInDeviceId != 0 && xmxAudioOutDeviceId != 0) break;
        }
    }

    if (xmxAudioInDeviceId == 0) {
        TraxHost::error("audio input device not found");
        return false;
    }

    if (xmxAudioOutDeviceId == 0) {
        TraxHost::error("audio output device not found");
        return false;
    }

    api.audioInDeviceId = xmxAudioInDeviceId;
    api.audioOutDeviceId = xmxAudioOutDeviceId;

    for (int i = 0; i < kAudioInCh; i++) { module.inputEnabled(i, 1); }
    for (int i = 0; i < kAudioOutCh; i++) { module.outputEnabled(i, 1); }
    return true;
}

bool startAudio(AudioApi &api) {
    RtAudio &audioApi = api.rtAudio_;

    RtAudio::StreamParameters iParams, oParams;
    iParams.nChannels = api.inCh;
    iParams.firstChannel = 0;
    oParams.nChannels = api.outCh;
    oParams.firstChannel = 0;

    iParams.deviceId = api.audioInDeviceId;
    oParams.deviceId = api.audioOutDeviceId;

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    if (audioApi.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, api.sampleRate, &api.bufSize, &audioCallback,
                            (void *)&api, &options)) {
        TraxHost::error("Failed to open audio stream");
        return false;
    }

    api.module->prepareAudioCallback(api.sampleRate, api.bufSize);

    if (!audioApi.isStreamOpen()) {
        TraxHost::error("Failed to open audio stream");
        return false;
    }
    if (audioApi.startStream()) {
        TraxHost::error("Failed to start audio stream");
        return false;
    }
    return true;
}

void stopAudio(AudioApi &api) {
    RtAudio &audioApi = api.rtAudio_;
    if (audioApi.isStreamRunning()) { audioApi.stopStream(); }
    if (audioApi.isStreamOpen()) { audioApi.closeStream(); }
}

}  // namespace TraxHost