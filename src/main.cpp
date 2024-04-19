
#include <SDL2/SDL.h>
#include <stdlib.h>

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 320

#include "SSPModule.h"


int main(int argc, char **argv) {
    SSPModule sspModule;
    if (sspModule.load("trax")) {
        printf("Loaded trax\n");
    } else {
        printf("Failed to load trax\n");
    }

    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_HEIGHT, WINDOW_WIDTH, 0, &window, &renderer);
    SDL_ShowCursor(false);

    SDL_Rect rect = { 0, 0, WINDOW_WIDTH, 240 };
    SDL_Surface *surface = SDL_CreateRGBSurface(0, rect.w, rect.h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 64, 64, 64));

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Event event;
    bool quit = false;
    sspModule.visibilityChanged(true);

    unsigned char argb[rect.w * rect.h * 4];

    while (!quit) {
        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_QUIT) { quit = true; }
        }

        Uint8 r = 0, g = 0, b = 0, a = 0;
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderClear(renderer);

        sspModule.frameStart();


        sspModule.renderToImage(argb, rect.w, rect.h);

        SDL_UpdateTexture(texture, NULL, argb, rect.w * 4);
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_RenderPresent(renderer);

        if (quit) { sspModule.visibilityChanged(false); }
    }

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
