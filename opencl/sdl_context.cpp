#include <iostream>

#include <SDL.h>

#include "sdl_context.hpp"
#include "random.hpp"

using namespace std;

SDL_Context::SDL_Context(int width, int height)
    : width_(width)
{
    int init = SDL_Init(SDL_INIT_VIDEO);
    if (init != 0) {
        cout << SDL_GetError() << endl;
        exit(1);
    }    

    win_ = nullptr;
    win_ = SDL_CreateWindow("Hello World!", 100, 100, width, height, SDL_WINDOW_SHOWN);
    if (win_ == nullptr) {
        cout << SDL_GetError() << endl;
        exit(1);
    }
    
    ren_ = nullptr;
    ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren_ == nullptr) {
        cout << SDL_GetError() << endl;
        exit(1);
    }

    tex_ = SDL_CreateTexture(ren_,
                             SDL_PIXELFORMAT_ARGB8888,
                             SDL_TEXTUREACCESS_STREAMING,
                             width, height);
        
    /*
    bmp_ = SDL_LoadBMP("background.bmp");
    if (bmp_ == nullptr) {
        cout << SDL_GetError() << endl;
        exit(1);
    }
        
    tex_ = SDL_CreateTextureFromSurface(ren_, bmp_);
    SDL_FreeSurface(bmp_);
    if (tex_ == nullptr) {
        cout << SDL_GetError() << endl;
        exit(1);
    }
    */

    pixels_ = static_cast<Uint32*>(malloc(sizeof(*pixels_) * width * height));
}

void SDL_Context::draw() {
    while(SDL_PollEvent(&event_))
        handle_event();

    draw_screen();

    SDL_UpdateTexture(tex_, NULL, pixels_, width_ * sizeof(Uint32));
    SDL_RenderClear(ren_);
    SDL_RenderCopy(ren_, tex_, NULL, NULL);
    SDL_RenderPresent(ren_);
}

SDL_Context::~SDL_Context() {
    SDL_DestroyTexture(tex_);
    SDL_DestroyRenderer(ren_);
    SDL_DestroyWindow(win_);
    SDL_Quit();
    free(pixels_);
}
