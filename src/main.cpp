
#include <SDL2/SDL.h>

#include <iostream>

#include "Hardware.h"
#include "Log.h"
#include "Module.h"
#include <RtAudio.h>
#include <signal.h>
#include <thread>

bool keepRunning = true;
static constexpr unsigned int kAudioBufSize = 512;
static constexpr unsigned int kAudioSampleRate = 48000;
static constexpr unsigned int kAudioInCh = 8;
static constexpr unsigned int kAudioOutCh = 2;

bool handleEvent(SDL_Event &event, TraxHost::Module &module) {
    switch (event.type) {
        case SDL_QUIT: return false;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: {
                    if (event.type == SDL_KEYUP) { return false; }
                    break;
                }
                case SDLK_UP: {
                    module.buttonPressed(TraxHost::Module::SSPButtons::SSP_Up, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_DOWN: {
                    module.buttonPressed(TraxHost::Module::SSPButtons::SSP_Down, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_LEFT: {
                    module.buttonPressed(TraxHost::Module::SSPButtons::SSP_Down, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_RIGHT: {
                    module.buttonPressed(TraxHost::Module::SSPButtons::SSP_Down, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_LEFTBRACKET: {
                    module.buttonPressed(TraxHost::Module::SSPButtons::SSP_Shift_L, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_RIGHTBRACKET: {
                    module.buttonPressed(TraxHost::Module::SSPButtons::SSP_Shift_R, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_1 ... SDLK_8: {
                    module.buttonPressed(TraxHost::Module::SSPButtons::SSP_Soft_1 + event.key.keysym.sym - SDLK_1,
                                         event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_q: {
                    if (event.type == SDL_KEYDOWN) module.encoderTurned(0, 1);
                    break;
                }
                case SDLK_w: {
                    if (event.type == SDL_KEYDOWN) module.encoderTurned(1, 1);
                    break;
                }
                case SDLK_e: {
                    if (event.type == SDL_KEYDOWN) module.encoderTurned(2, 1);
                    break;
                }
                case SDLK_r: {
                    if (event.type == SDL_KEYDOWN) module.encoderTurned(3, 1);
                    break;
                }
                case SDLK_a: {
                    if (event.type == SDL_KEYDOWN) module.encoderTurned(0, -1);
                    break;
                }
                case SDLK_s: {
                    if (event.type == SDL_KEYDOWN) module.encoderTurned(1, -1);
                    break;
                }
                case SDLK_d: {
                    if (event.type == SDL_KEYDOWN) module.encoderTurned(2, -1);
                    break;
                }
                case SDLK_f: {
                    if (event.type == SDL_KEYDOWN) module.encoderTurned(3, -1);
                    break;
                }
                case SDLK_z: {
                    module.encoderPressed(0, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_x: {
                    module.encoderPressed(1, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_c: {
                    module.encoderPressed(2, event.type == SDL_KEYDOWN);
                    break;
                }
                case SDLK_v: {
                    module.encoderPressed(3, event.type == SDL_KEYDOWN);
                    break;
                }
            }  // switch(event.key.keysym.sym)
            break;
        }  // case SDL_KEYDOWN, SDL_KEYUP
    }  // switch(event.type)
    return true;
}


bool handleHardwareEvents(TraxHost::Hardware &hw, TraxHost::Module &module) {
    return hw.handleEvents(module);
}


void intHandler(int sig) {
    // only called in main thread
    if (sig == SIGINT) { keepRunning = 0; }
}


struct AudioData {
    unsigned audioDeviceId = 0;
    unsigned sampleRate = kAudioSampleRate;
    unsigned bufSize = kAudioBufSize;
    unsigned inCh = kAudioInCh;
    unsigned outCh = kAudioOutCh;

    RtAudio *audioApi_ = nullptr;
    TraxHost::Module *module = nullptr;
};


int inout(void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/, double streamTime,
          RtAudioStreamStatus status, void *data) {
    if (status) TraxHost::error(std::string("Stream over/underflow detected."));

    AudioData *audioData = (AudioData *)data;
    audioData->module->audioCallback((float *)inputBuffer, audioData->inCh, (float *)outputBuffer, audioData->outCh,
                                     audioData->bufSize);
    return 0;
}

bool startAudio(AudioData &audioData) {
    RtAudio &audioApi = *audioData.audioApi_;

    RtAudio::StreamParameters iParams, oParams;
    iParams.nChannels = audioData.inCh;
    iParams.firstChannel = 0;
    oParams.nChannels = audioData.outCh;
    oParams.firstChannel = 0;

    iParams.deviceId = audioData.audioDeviceId;
    oParams.deviceId = audioData.audioDeviceId;


    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    if (audioApi.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, audioData.sampleRate, &audioData.bufSize, &inout,
                            (void *)&audioData, &options)) {
        TraxHost::error("Failed to open audio stream");
        return false;
    }

    audioData.module->prepareAudioCallback(audioData.sampleRate, audioData.bufSize);

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

int main(int argc, char **argv) {
    TraxHost::log("TraxHost: Starting");

#ifndef __APPLE__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        TraxHost::error("TTraxHost,  Error calling pthread_setaffinity_np: " + std::to_string(rc));
    } else {
        TraxHost::log("TraxHost, Set affinity for main thread");
    }
#endif    

    RtAudio audioApi;
    // prints to stderr
    audioApi.showWarnings(true);

    std::vector<unsigned int> ids = audioApi.getDeviceIds();
    RtAudio::DeviceInfo info;

#ifdef __APPLE__
    static constexpr const char *audioDevNamePrefix = "XMX";
#else
    static constexpr const char *audioDevNamePrefix = "rockchip";
#endif

    unsigned xmxAudioDeviceId = 0;
    for (unsigned int n = 0; n < ids.size(); n++) {
        info = audioApi.getDeviceInfo(ids[n]);
        if (info.name.find(audioDevNamePrefix) != std::string::npos) {
            xmxAudioDeviceId = ids[n];
            TraxHost::log(std::string("found audio device: ") + info.name + std::string(", id: ") +
                          std::to_string(ids[n]));
            TraxHost::log(std::string("out: ") + std::to_string(info.outputChannels) + std::string(", in: ") +
                          std::to_string(info.inputChannels));
            if (info.outputChannels < 2 || info.inputChannels < 8) {
                TraxHost::error("audio device does not have enough channels");
                return -1;
            }
            break;
        }
    }

    if (xmxAudioDeviceId == 0) {
        TraxHost::error("audio device not found");
        return -1;
    }


#ifndef WIN32
    // block sigint from other threads
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
#endif


#ifndef WIN32
    // Install the signal handler for SIGINT.
    struct sigaction s;
    s.sa_handler = intHandler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);
    // Restore the old signal mask only for this thread.
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
#endif

#if __APPLE__
    static constexpr unsigned devWidth = 320, devHeight = 240;
#else
    static constexpr unsigned devWidth = 320, devHeight = 320;
#endif

    static constexpr unsigned winWidth = devWidth, winHeight = 240;
    static constexpr unsigned pixelSize = 4;

    unsigned char argbBuffer[winWidth * winHeight * pixelSize];

    TraxHost::Hardware hw;

    if (hw.init()) {
        TraxHost::log("Hardware init success");
    } else {
        TraxHost::error("Hardware init failed");
        return -1;
    }

    TraxHost::Module module;
    if (module.load("trax")) {
        TraxHost::log("Loaded trax");
    } else {
        TraxHost::error("Loading failed trax");
        return -1;
    }


    AudioData audioData;
    audioData.audioDeviceId = xmxAudioDeviceId;
    audioData.audioApi_ = &audioApi;
    audioData.module = &module;
    for(int i=0; i<kAudioInCh; i++) {
        module.inputEnabled(i, 1);
    }
    for(int i=0; i<kAudioOutCh; i++) {
        module.outputEnabled(i, 1);
    }

    if (!startAudio(audioData)) {
        TraxHost::error("Failed to start audio");
        return -1;
    }


    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Init(0);
    SDL_CreateWindowAndRenderer(devWidth, devHeight, 0, &window, &renderer);
    SDL_ShowCursor(SDL_DISABLE);

    SDL_Rect winRect = { 0, 0, winWidth, winHeight };
    SDL_Texture *texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, winRect.w, winRect.h);

    module.visibilityChanged(true);

    TraxHost::log("TraxHost: Running");
    while (keepRunning) {
#if __APPLE__
        // SDL_PollEvent eats all IO events
        SDL_Event event;
        while (SDL_PollEvent(&event)) { keepRunning = handleEvent(event, module); }
#endif
        handleHardwareEvents(hw, module);

        Uint8 r = 0, g = 0, b = 0, a = 0;
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderClear(renderer);

        module.frameStart();
        module.renderToImage(argbBuffer, winRect.w, winRect.h);

        SDL_UpdateTexture(texture, NULL, argbBuffer, winRect.w * 4);
        SDL_RenderCopy(renderer, texture, NULL, &winRect);
        SDL_RenderPresent(renderer);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/30)); // 30fps
    }
    TraxHost::log("TraxHost: Shutting down");

    module.visibilityChanged(false);

    if (audioApi.isStreamRunning()) audioApi.stopStream();


#ifndef WIN32
    // been told to stop, block SIGINT, to allow clean termination
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
#endif

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TraxHost::log("TraxHost: Shutdown");
    return 0;
}
