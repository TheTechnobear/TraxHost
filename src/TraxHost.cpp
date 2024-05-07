
#include <signal.h>

#include <iostream>

#include "AudioDevice.h"
#include "Display.h"
#include "Hardware.h"
#include "Log.h"
#include "Module.h"

bool keepRunning = true;

void intHandler(int sig) {
    // only called in main thread
    if (sig == SIGINT) { keepRunning = 0; }
}

int main(int argc, char **argv) {
    TraxHost::log("TraxHost: Starting");

    TraxHost::Module module;
    TraxHost::Hardware hw;
    TraxHost::AudioApi audioApi;

    auto display = createDisplay(devWidth, devHeight, winWidth, winHeight);

#ifndef __APPLE__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        TraxHost::error("TraxHost,  Error calling pthread_setaffinity_np: " + std::to_string(rc));
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

    std::string modulefile = "trax";
    std::string presetfile = modulefile + "_presets/default";
    std::string pluginDir = "./plugins";

    if (argc > 1) { modulefile = argv[1]; }
    if (argc > 2) { presetfile = argv[2]; }
    if (argc > 3) { pluginDir = argv[3]; }

    TraxHost::log("TraxHost using : " + modulefile + " with preset : " + presetfile +
                  " using plugin dir : " + pluginDir);

    if (module.load(pluginDir, modulefile)) {
        TraxHost::log("Loaded " + modulefile);
    } else {
        TraxHost::error("Loading failed for " + modulefile);
        return -1;
    }
    module.loadPreset(presetfile);

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

    if (display->init()) {
        TraxHost::log("Display init success");
    } else {
        TraxHost::error("Display init failed");
        return -1;
    }

    module.visibilityChanged(true);

    TraxHost::log("TraxHost: Running");
    while (keepRunning) {
        if (!display->handleEvents(module)) { keepRunning = false; }
        hw.handleEvents(module);

        module.frameStart();
        module.renderToImage(display->getBuffer(), display->getWinWidth(), display->getWinHeight());
        display->update();
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
    display.reset();

    TraxHost::log("TraxHost: Shutdown");
    return 0;
}
