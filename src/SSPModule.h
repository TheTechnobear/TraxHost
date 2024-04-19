#pragma once

#include "../../ssp-sdk/Percussa.h"

#include <atomic>

class AudioCallback {
public:
    AudioCallback() = default;
    virtual ~AudioCallback() = default;
    void prepareAudioCallback(int numInputChannels, int numOutputChannels, int numSamples);
    void audioCallback(const float *const *inputChannelData, int numInputChannels, float *const *outputChannelData,
                       int numOutputChannels, int numSamples);
};

class SSPModule : public AudioCallback {
public:
    explicit SSPModule() = default;
    ~SSPModule() override {
        free();
    }

    bool load(std::string pluginName);


    void frameStart();
	void visibilityChanged(bool b);
	void renderToImage(unsigned char* buffer, int width, int height);


    void prepareAudioCallback(int numInputChannels, int numOutputChannels, int sampleRate, int numSamples);
    void audioCallback(const float *const *inputChannelData, int numInputChannels, float *const *outputChannelData,
                       int numOutputChannels, int numSamples);
private:
    void alloc(const std::string &f, Percussa::SSP::PluginInterface *p, Percussa::SSP::PluginDescriptor *d, void *h);
    void free();

    std::string pluginName_;
    std::string pluginFile_;
    std::string requestedModule_;
    std::atomic_flag lockModule_ = ATOMIC_FLAG_INIT;
    Percussa::SSP::PluginInterface *plugin_ = nullptr;
    Percussa::SSP::PluginDescriptor *descriptor_ = nullptr;
    Percussa::SSP::PluginEditorInterface *editor_ = nullptr;
    
    void *dlHandle_ = nullptr;

    float **audioSampleBuffer_;
    bool activeDevice_ = false;
    int sampleRate_ = 48000;
    int bufferSize_ = 512;

    int nChIn_ = 2;
    int nChOut_ = 8;
    int maxCh_ = std::max(nChIn_, nChOut_);

};