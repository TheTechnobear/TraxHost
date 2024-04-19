#include "SSPModule.h"

#include <dlfcn.h>


void SSPModule::alloc(const std::string &f, Percussa::SSP::PluginInterface *p, Percussa::SSP::PluginDescriptor *d,
                      void *h) {
    pluginFile_ = f;
    plugin_ = p;
    editor_ = nullptr;
    descriptor_ = d;
    dlHandle_ = h;
    requestedModule_ = f;
}


void SSPModule::free() {
    if (plugin_) delete plugin_;
    if (descriptor_) delete descriptor_;
    // if (editor_) delete editor_; // owned by plugin

    if (dlHandle_) dlclose(dlHandle_);

    plugin_ = nullptr;
    dlHandle_ = nullptr;
    descriptor_ = nullptr;
    pluginFile_ = "";
    requestedModule_ = "";
}


bool SSPModule::load(std::string pluginName) {
    free();
    pluginName_ = pluginName;
    if (pluginName_ == "") {
        /// just cleared this module
        return true;
    }

#ifdef __APPLE__
    const static int dlopenmode = RTLD_LOCAL | RTLD_NOW;
    pluginFile_ = "/Users/kodiak/Library/Audio/Plug-Ins/VST3/" + pluginName + ".vst3/Contents/MacOS/" + pluginName;
    // pluginFile_ = pluginName + ".vst3/Contents/MacOS/" + pluginName;
#else
    const static int dlopenmode = RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND;
    pluginFile_ = "plugins/" + pluginName + ".so";
#endif

    auto fHandle = dlopen(pluginFile_.c_str(), dlopenmode);
    auto fnVersion = (Percussa::SSP::VersionFun)dlsym(fHandle, Percussa::SSP::getApiVersionName);
    auto fnCreateDescriptor = (Percussa::SSP::DescriptorFun)dlsym(fHandle, Percussa::SSP::createDescriptorName);
    auto fnCreateInterface = (Percussa::SSP::InstantiateFun)dlsym(fHandle, Percussa::SSP::createInstanceName);

    if (fHandle) {
        auto fnVersion = (Percussa::SSP::VersionFun)dlsym(fHandle, Percussa::SSP::getApiVersionName);
        auto fnCreateInterface = (Percussa::SSP::InstantiateFun)dlsym(fHandle, Percussa::SSP::createInstanceName);
        auto fnCreateDescriptor = (Percussa::SSP::DescriptorFun)dlsym(fHandle, Percussa::SSP::createDescriptorName);

        if (fnCreateInterface && fnCreateDescriptor) {
            auto desc = fnCreateDescriptor();
            auto plugin = fnCreateInterface();
            alloc(pluginFile_, plugin, desc, fHandle);

            // prepare for play
            int inSz = descriptor_->inputChannelNames.size();
            int outSz = descriptor_->outputChannelNames.size();

            if (sampleRate_ != 0) { prepareAudioCallback(inSz, outSz, sampleRate_, bufferSize_); }

            return true;
        }
        dlclose(fHandle);
    }
    return false;
}


void SSPModule::prepareAudioCallback(int numInputChannels, int numOutputChannels, int sampleRate, int numSamples) {
    sampleRate_ = sampleRate;
    bufferSize_ = numSamples;
    activeDevice_ = true;

    int nCh = std::max(numInputChannels, numOutputChannels);
    if (nCh != maxCh_) {
        for (int i = 0; i < maxCh_; i++) { delete audioSampleBuffer_[i]; }
        delete audioSampleBuffer_;

        audioSampleBuffer_ = new float *[maxCh_];
        for (int i = 0; i < maxCh_; i++) { audioSampleBuffer_[i] = new float[numSamples]; }
    }
    nChIn_ = numInputChannels;
    nChOut_ = numOutputChannels;
    maxCh_ = nCh;

    if (plugin_) { plugin_->prepare(sampleRate, numSamples); }
}


void SSPModule::audioCallback(const float *const *inputChannelData, int numInputChannels,
                              float *const *outputChannelData, int numOutputChannels, int numSamples) {
    if (activeDevice_ && plugin_) {
        int inSz = descriptor_->inputChannelNames.size();
        int outSz = descriptor_->outputChannelNames.size();

        for (int i = 0; i < inSz; i++) {
            for (int j = 0; j < numSamples; j++) {
                audioSampleBuffer_[i][j] = i < numInputChannels ? inputChannelData[i][j] : 0.0f;
            }
        }

        plugin_->process(audioSampleBuffer_, maxCh_, numSamples);

        for (int i = 0; i < numOutputChannels; i++) {
            for (int j = 0; j < numSamples; j++) {
                outputChannelData[i][j] = i < outSz ? audioSampleBuffer_[i][j] : 0.0f;
            }
        }
        return;
    }
    // no valid plugin
    for (int i = 0; i < numOutputChannels; i++) {
        for (int j = 0; j < numSamples; j++) { outputChannelData[i][j] = 0.0f; }
    }
    return;
}


void SSPModule::frameStart() {
    if (!editor_) editor_ = plugin_->getEditor();
    if (editor_) editor_->frameStart();
}

void SSPModule::visibilityChanged(bool b) {
    if (!editor_) editor_ = plugin_->getEditor();
    if (editor_) editor_->visibilityChanged(b);
}

void SSPModule::renderToImage(unsigned char *buffer, int width, int height) {
    if (!editor_) editor_ = plugin_->getEditor();
    if (editor_) editor_->renderToImage(buffer, width, height);
}
