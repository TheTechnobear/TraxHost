#include "Display.h"

#include "Log.h"

Display::Display(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes)
    : xDevRes_(xDevRes), yDevRes_(yDevRes), xWinRes_(xWinRes), yWinRes_(yWinRes) {
    argbBuffer_ = new unsigned char[xDevRes_ * yDevRes_ * pixelSize];
}


Display::~Display() {
    delete[] argbBuffer_;
}

///// SDL Implementation ---------

#if USE_SDL

std::shared_ptr<Display> createDisplay(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes) {
    return std::make_shared<SDLDisplay>(xDevRes, yDevRes, xWinRes, yWinRes);
}

SDLDisplay::SDLDisplay(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes)
    : Display(xDevRes, yDevRes, xWinRes, yWinRes) {
}

SDLDisplay::~SDLDisplay() {
    SDL_DestroyTexture(texture_);
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool SDLDisplay::init() {
    SDL_Init(0);
    SDL_CreateWindowAndRenderer(xDevRes_, yDevRes_, 0, &window_, &renderer_);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_Rect win = { 0, 0, (int)xWinRes_, (int)yWinRes_ };
    winRect_ = win;
    texture_ =
        SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, winRect_.w, winRect_.h);
    return true;
}

void SDLDisplay::update() {
    SDL_UpdateTexture(texture_, &winRect_, argbBuffer_, xWinRes_ * pixelSize);
    SDL_RenderCopy(renderer_, texture_, NULL, &winRect_);
    SDL_RenderPresent(renderer_);
}

bool SDLDisplay::handleEvents(TraxHost::Module &module) {
#if __APPLE__
    bool ret = true;
    SDL_Event event;
    while (SDL_PollEvent(&event)) { ret = handleEvent(event, module); }
    return ret;
#else
    // render at given frame rate...
    // (change later to mutex to allow for timely exit from sigint)
    std::this_thread::sleep_for(std::chrono::milliseconds(frDelayMS));
    return true;
#endif
}


bool SDLDisplay::handleEvent(SDL_Event &event, TraxHost::Module &module) {
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

#else
std::shared_ptr<Display> createDisplay(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes) {
    return std::make_shared<FBDisplay>(xDevRes, yDevRes, xWinRes, yWinRes);
}

#include <fcntl.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

FBDisplay::FBDisplay(unsigned xDevRes, unsigned yDevRes, unsigned xWinRes, unsigned yWinRes)
    : Display(xDevRes, yDevRes, xWinRes, yWinRes) {
}

FBDisplay::~FBDisplay() {
    if (fbfd > 0) {
        munmap(fbdata, fb_data_size);
        close(fbfd);
        fbfd = -1;
    }
}


bool FBDisplay::init() {
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        TraxHost::error("Error: cannot open framebuffer device");
        return false;
    }

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info) == -1) {
        TraxHost::error("Error getting fixed screen information");
        close(fbfd);
        fbfd = -1;
        return false;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &var_info) == -1) {
        TraxHost::error("Error getting variable screen information");
        close(fbfd);
        fbfd = -1;
        return false;
    }

    fb_width = var_info.xres;
    fb_height = var_info.yres;
    fb_bpp = var_info.bits_per_pixel;
    fb_bytes = fb_bpp / 8;

    TraxHost::log("Framebuffer resolution: " + std::to_string(fb_width) + "x" + std::to_string(fb_height) + "x" +
                  std::to_string(fb_bpp) + "bpp");

    fb_data_size = fb_width * fb_height * fb_bytes;
    fbdata = (char *)mmap(nullptr, fb_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbdata == MAP_FAILED) {
        TraxHost::error("Error mapping framebuffer memory");
        close(fbfd);
        fbfd = -1;
        return false;
    }

    memset(fbdata, 0, fb_data_size);

    return true;
}

void FBDisplay::update() {
    memcpy(fbdata, argbBuffer_, fb_data_size);
}

bool FBDisplay::handleEvents(TraxHost::Module &module) {
    std::this_thread::sleep_for(std::chrono::milliseconds(frDelayMS));
    return true;
}


#endif
