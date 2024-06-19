#pragma once

#if __APPLE__
static constexpr unsigned devWidth = 320, devHeight = 240;
static constexpr unsigned winWidth = devWidth, winHeight = 240;
#elif defined(TARGET_SSP)
static constexpr unsigned devWidth = 1600, devHeight = 480;
static constexpr unsigned winWidth = devWidth, winHeight = 240 * 2;
#else
static constexpr unsigned devWidth = 320, devHeight = 320;
static constexpr unsigned winWidth = devWidth, winHeight = 240;
#endif
static constexpr unsigned pixelSize = 4;
static constexpr unsigned frameRate = 30;
static constexpr unsigned frDelayMS = 1000 / frameRate;

#include <chrono>
#include <thread>

#include "Module.h"

class Display {
public:
    Display(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes);
    virtual ~Display();
    virtual bool init() = 0;
    virtual void update() = 0;
    virtual bool handleEvents(TraxHost::Module &module) = 0;
    unsigned getDevWidth() { return xDevRes_; }
    unsigned getDevHeight() { return yDevRes_; }
    unsigned getWinWidth() { return xWinRes_; }
    unsigned getWinHeight() { return yWinRes_; }


    unsigned char *getBuffer() { return argbBuffer_; }

protected:
    unsigned xDevRes_ = 0, yDevRes_ = 0;
    unsigned xWinRes_ = 0, yWinRes_ = 0;
    unsigned char *argbBuffer_;
};

std::shared_ptr<Display> createDisplay(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes);


#ifdef USE_SDL


#include <SDL2/SDL.h>

class SDLDisplay : public Display {
public:
    SDLDisplay(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes);
    virtual ~SDLDisplay();
    bool init() override;
    void update() override;
    bool handleEvents(TraxHost::Module &module) override;

private:
    bool handleEvent(SDL_Event &event, TraxHost::Module &module);

    SDL_Renderer *renderer_;
    SDL_Window *window_;
    SDL_Rect winRect_;
    SDL_Texture *texture_;
};


#else

#include <linux/fb.h>

class FBDisplay : public Display {
public:
    FBDisplay(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes);
    virtual ~FBDisplay();
    bool init() override;
    void update() override;
    bool handleEvents(TraxHost::Module &module) override;

private:
    int fbfd = 0;
    struct fb_fix_screeninfo fix_info;
    struct fb_var_screeninfo var_info;
    int fb_width = 0;
    int fb_height = 0;
    int fb_bpp = 0;
    int fb_bytes = 0;
    int fb_data_size = 0;
    char *fbdata = 0;
};


#endif