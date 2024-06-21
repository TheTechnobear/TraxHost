#include "Module.h"

#include <dlfcn.h>

#include "AudioDevice.h"

namespace TraxHost {


void Module::alloc(const std::string &f, Percussa::SSP::PluginInterface *p, Percussa::SSP::PluginDescriptor *d,
                   void *h) {
    pluginFile_ = f;
    plugin_ = p;
    editor_ = nullptr;
    descriptor_ = d;
    dlHandle_ = h;
    requestedModule_ = f;

    nChIn_ = descriptor_->inputChannelNames.size();
    nChOut_ = descriptor_->outputChannelNames.size();
    int nCh = std::max(nChIn_, nChOut_);
    allocateAudioBuffers(nCh, bufferSize_);
}


void Module::free() {
    if (plugin_) delete plugin_;
    if (descriptor_) delete descriptor_;
    // if (editor_) delete editor_; // owned by plugin

    if (dlHandle_) dlclose(dlHandle_);

    plugin_ = nullptr;
    dlHandle_ = nullptr;
    descriptor_ = nullptr;
    pluginFile_ = "";
    requestedModule_ = "";
    freeAudioBuffers();
    activeDevice_ = false;
}


void Module::freeAudioBuffers() {
    if (audioBuffer_ != nullptr) {
        for (int i = 0; i < audioBufferChannels_; i++) { delete audioBuffer_[i]; }
        delete audioBuffer_;
        audioBuffer_ = nullptr;
        audioBufferChannels_ = 0;
    }
}

void Module::allocateAudioBuffers(int ch, int numSamples) {
    if (audioBuffer_ != nullptr) { freeAudioBuffers(); }

    audioBufferChannels_ = ch;
    bufferSize_ = numSamples;
    audioBuffer_ = new float *[audioBufferChannels_];
    for (int i = 0; i < audioBufferChannels_; i++) { audioBuffer_[i] = new float[numSamples]; }
    for (int i = 0; i < audioBufferChannels_; i++) {
        for (int j = 0; j < bufferSize_; j++) { audioBuffer_[i][j] = 0.0f; }
    }
}

bool Module::load(const std::string& pluginDir, const std::string& pluginName) {
    free();
    pluginName_ = pluginName;
    if (pluginName_ == "") {
        /// just cleared this module
        return true;
    }

#ifdef __APPLE__
    static constexpr int dlopenmode = RTLD_LOCAL | RTLD_NOW;
    pluginFile_ = pluginDir + std::string("/")  + pluginName + ".vst3/Contents/MacOS/" + pluginName;
#else
    static constexpr int dlopenmode = RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND;
    pluginFile_ = pluginDir + std::string("/") + pluginName + ".so";
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
            if (sampleRate_ != 0) { prepareAudioCallback(sampleRate_, bufferSize_); }

            return true;
        }
        dlclose(fHandle);
    }
    return false;
}


void Module::prepareAudioCallback(int sampleRate, int numSamples) {
    sampleRate_ = sampleRate;
    activeDevice_ = true;

    if (numSamples != bufferSize_) { allocateAudioBuffers(audioBufferChannels_, numSamples); }
    if (plugin_) { plugin_->prepare(sampleRate_, bufferSize_); }
}


void Module::audioCallback(float *inputChannelData, int numInputChannels, float *outputChannelData,
                           int numOutputChannels, int numSamples) {
    if (activeDevice_ && plugin_) {
        if (numSamples != bufferSize_ || audioBuffer_ == nullptr) {
            // should not happen!
            freeAudioBuffers();
            allocateAudioBuffers(audioBufferChannels_, numSamples);
        }


        for (int i = 0; i < nChIn_; i++) {
            int inCh =  i < numInputChannels ?  kInChMap[i] : -1 ; 
            for (int j = 0; j < bufferSize_; j++) {
                audioBuffer_[i][j] = inCh < numInputChannels && inCh >= 0 ? inputChannelData[inCh * bufferSize_ + j] : 0.0f;
            }
        }

        plugin_->process(audioBuffer_, audioBufferChannels_, numSamples);

        for (int i = 0; i < numOutputChannels; i++) {
            int outCh = kOutChMap[i]; 
            for (int j = 0; j < bufferSize_; j++) {
                outputChannelData[outCh * bufferSize_ + j] = i < nChOut_ ? audioBuffer_[i][j] : 0.0f;
            }
        }


        return;
    }
    // no valid plugin
    for (int i = 0; i < numOutputChannels; i++) {
        for (int j = 0; j < numSamples; j++) { outputChannelData[i * bufferSize_ + j] = 0.0f; }
    }
    return;
}


void Module::frameStart() {
    if (!editor_) editor_ = plugin_->getEditor();
    if (editor_) editor_->frameStart();
}

void Module::visibilityChanged(bool b) {
    if (!editor_) editor_ = plugin_->getEditor();
    if (editor_) editor_->visibilityChanged(b);
}

void Module::renderToImage(unsigned char *buffer, int width, int height) {
    if (!editor_) editor_ = plugin_->getEditor();
    if (editor_) editor_->renderToImage(buffer, width, height);
}


void Module::loadPreset(const std::string &pr) {
    if (!plugin_) return;

    auto file = fopen(pr.c_str(), "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        auto size = ftell(file);
        fseek(file, 0, SEEK_SET);
        auto data = new char[size];
        fread(data, 1, size, file);
        fclose(file);
        plugin_->setState(data, size);
        delete[] data;
    }
}


}  // namespace TraxHost
