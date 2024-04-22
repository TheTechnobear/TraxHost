
#include <SDL2/SDL.h>
#include <signal.h>

#include <iostream>
#include <thread>

#include "AudioDevice.h"
#include "Hardware.h"
#include "Log.h"
#include "Module.h"

bool keepRunning = true;

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


void intHandler(int sig) {
    // only called in main thread
    if (sig == SIGINT) { keepRunning = 0; }
}

int main(int argc, char **argv) {
    TraxHost::log("TraxHost: Starting");

    TraxHost::Module module;
    TraxHost::Hardware hw;
    TraxHost::AudioApi audioApi;
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

#ifndef WIN32
    // block sigint from other threads
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);

    // Install the signal handler for SIGINT.
    struct sigaction s;
    s.sa_handler = intHandler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);

    // Restore the old signal mask only for this thread.
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

#endif

    if (hw.init()) {
        TraxHost::log("Hardware init success");
    } else {
        TraxHost::error("Hardware init failed");
        return -1;
    }

    if (module.load("trax")) {
        TraxHost::log("Loaded trax");
    } else {
        TraxHost::error("Loading failed trax");
        return -1;
    }
    module.loadPreset("presets/default");

    if (initAudio(audioApi, module)) {
        TraxHost::log("Audio init success");
    } else {
        TraxHost::error("Audio init failed");
        return -1;
    }

    if (!startAudio(audioApi)) {
        TraxHost::error("Failed to start audio");
        return -1;
    }


#if __APPLE__
    static constexpr unsigned devWidth = 320, devHeight = 240;
#else
    static constexpr unsigned devWidth = 320, devHeight = 320;
#endif
    static constexpr unsigned winWidth = devWidth, winHeight = 240;
    static constexpr unsigned pixelSize = 4;
    static constexpr unsigned frameRate = 30;
    static constexpr unsigned frDelayMS = 1000 / frameRate;
    unsigned char argbBuffer[winWidth * winHeight * pixelSize];

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

        hw.handleEvents(module);

        module.frameStart();
        module.renderToImage(argbBuffer, winRect.w, winRect.h);

        SDL_UpdateTexture(texture, NULL, argbBuffer, winRect.w * 4);
        SDL_RenderCopy(renderer, texture, NULL, &winRect);
        SDL_RenderPresent(renderer);

        // render at given frame rate...
        // (change later to mutex to allow for timely exit from sigint)
        std::this_thread::sleep_for(std::chrono::milliseconds(frDelayMS));
    }

#ifndef WIN32
    // been told to stop, block SIGINT, to allow clean termination
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
#endif

    TraxHost::log("TraxHost: Shutting down");
    module.visibilityChanged(false);

    stopAudio(audioApi);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TraxHost::log("TraxHost: Shutdown");
    return 0;
}
