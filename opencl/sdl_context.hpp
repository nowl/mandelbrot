#ifndef __SDL_CONTEXT__
#define __SDL_CONTEXT__

#include <SDL.h>

class SDL_Context {
private:
    SDL_Window *win_;
    SDL_Renderer *ren_;
    SDL_Surface *bmp_;
    SDL_Texture *tex_;
    int width_;
    
public:
    SDL_Context(int width, int height);
    ~SDL_Context();

    void draw();

protected:
    Uint32 *pixels_;
    SDL_Event event_;
    virtual void draw_screen() = 0;
    virtual void handle_event() = 0;
};

#endif  // __SDL_CONTEXT__
