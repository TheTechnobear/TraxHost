#pragma once

#include <atomic>

#include "../../ssp-sdk/Percussa.h"


namespace TraxHost {


class AudioCallback {
public:
    AudioCallback() = default;
    virtual ~AudioCallback() = default;
    void prepareAudioCallback(int sampleRate, int numSamples);
    void audioCallback(float *inputChannelData, int numInputChannels, float *outputChannelData, int numOutputChannels,
                       int numSamples);
};

class Module : public AudioCallback {
public:
    explicit Module() = default;
    ~Module() override { free(); }

    bool load(const std::string& pluginDir, const std::string& pluginName);


    void frameStart();
    void visibilityChanged(bool b);
    void encoderPressed(int encoder, int value) {
        if (plugin_) plugin_->encoderPressed(encoder, value);
    }
    void encoderTurned(int encoder, int value) {
        if (plugin_) plugin_->encoderTurned(encoder, value);
    }
    void buttonPressed(int button, int value) {
        if (plugin_) plugin_->buttonPressed(button, value);
    }
    void inputEnabled(int input, int value) {
        if (plugin_) plugin_->inputEnabled(input, value);
    }
    void outputEnabled(int output, int value) {
        if (plugin_) plugin_->outputEnabled(output, value);
    }


    void renderToImage(unsigned char *buffer, int width, int height);


    void prepareAudioCallback(int sampleRate, int numSamples);
    void audioCallback(float *inputChannelData, int numInputChannels, float *outputChannelData, int numOutputChannels,
                       int numSamples);

    void loadPreset(const std::string &pr);

    enum SSPButtons {
        SSP_Soft_1,
        SSP_Soft_2,
        SSP_Soft_3,
        SSP_Soft_4,
        SSP_Soft_5,
        SSP_Soft_6,
        SSP_Soft_7,
        SSP_Soft_8,
        SSP_Left,
        SSP_Right,
        SSP_Up,
        SSP_Down,
        SSP_Shift_L,
        SSP_Shift_R,
        SSP_LastBtn
    };


private:
    void alloc(const std::string &f, Percussa::SSP::PluginInterface *p, Percussa::SSP::PluginDescriptor *d, void *h);
    void free();

    void freeAudioBuffers();
    void allocateAudioBuffers(int ch, int numSamples);

    std::string pluginName_;
    std::string pluginFile_;
    std::string requestedModule_;
    std::atomic_flag lockModule_ = ATOMIC_FLAG_INIT;
    Percussa::SSP::PluginDescriptor *descriptor_ = nullptr;
    Percussa::SSP::PluginInterface *plugin_ = nullptr;
    Percussa::SSP::PluginEditorInterface *editor_ = nullptr;

    void *dlHandle_ = nullptr;

    bool activeDevice_ = false;
    int sampleRate_ = 48000;
    int bufferSize_ = 0;

    int nChIn_ = 2;
    int nChOut_ = 8;
    int audioBufferChannels_ = 0;
    float **audioBuffer_ = nullptr;
};

}  // namespace TraxHost
