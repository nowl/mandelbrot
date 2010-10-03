#include <stdlib.h>
#include <SDL.h>

static SDL_Surface *screen, *origin;
static int screen_width, screen_height;
static float re_min = -2;
static float re_max = 1;
static float im_min = -1;
static float im_max = 1;
static float x0_inc, y0_inc;
static int max_iters = 1000;
static stale = 1;
static last_display;
static int start_pick_x, start_pick_y, end_pick_x, end_pick_y;

void sdl_set_video_mode(unsigned int screen_width,
                        unsigned int screen_height,
                        unsigned char fullscreen)
{
    int flags = SDL_SWSURFACE|SDL_DOUBLEBUF;
    if(fullscreen)
        flags |= SDL_FULLSCREEN;
    screen = SDL_SetVideoMode(screen_width, screen_height, 32, flags);
    if ( !screen )
    {
        printf("Unable to set video mode: %s\n", SDL_GetError());
        exit(1);
    }
}

void putpixel(int x, int y, Uint32 pixel)
{
    SDL_Surface *surface = screen;

    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

void generate_mandel()
{
    int image_x, image_y;
    float x0, y0;

    for(image_y = 0, image_x = 0, x0 = re_min; image_x < screen_width; image_x++, x0 += x0_inc, image_y = 0)
    {
        for(y0 = im_min; image_y < screen_height; y0 += y0_inc, image_y++)
        {
            int iters = 0;
            float x = 0, y = 0;
            while( (x*x+y*y < 4) && (iters < max_iters) )
            {
                float xt = x*x - y*y + x0;
                y = 2*x*y + y0;
                x = xt;
                iters++;
            }

            if(iters >= max_iters)
                putpixel(image_x, image_y, 0);
            else {
                float smooth =  iters + 1 - log(log(sqrt(x*x+y*y)))/log(2);
                //float norm_iters = (float)iters / max_iters;
                putpixel(image_x, image_y, SDL_MapRGB(screen->format, 0, 0, smooth * 10));
            }
        }

        /* flip screen after 1 second */
        if(SDL_GetTicks() - last_display > 1000) {
            SDL_Flip(screen);
            last_display = SDL_GetTicks();
        }        
    }
}

void update_coords()
{
    /* linearly interpolate to get coords from picked locations */
    
    float new_re_min, new_re_max;
    float new_im_min, new_im_max;
    
    new_re_min = (float)start_pick_x / screen_width * (re_max - re_min) + re_min;
    new_re_max = (float)end_pick_x / screen_width * (re_max - re_min) + re_min;
    new_im_min = (float)start_pick_y / screen_height * (im_max - im_min) + im_min;
    new_im_max = (float)end_pick_y / screen_height * (im_max - im_min) + im_min;

    re_min = new_re_min;
    re_max = new_re_max;
    im_min = new_im_min;
    im_max = new_im_max;

    printf("new coords (%f, %f) to (%f, %f)\n", re_min, im_max, re_max, im_max);
}

void draw_box()
{
    if(start_pick_x > end_pick_x) {
        int tmp = start_pick_x;
        start_pick_x = end_pick_x;
        end_pick_x = tmp;
    }

    if(start_pick_y > end_pick_y) {
        int tmp = start_pick_y;
        start_pick_y = end_pick_y;
        end_pick_y = tmp;
    }

    int color = SDL_MapRGB(screen->format, 250, 0, 0);
    int x, y;

    for(x = start_pick_x; x < end_pick_x; x++)
        putpixel(x, start_pick_y, color);
    for(x = start_pick_x; x < end_pick_x; x++)
        putpixel(x, end_pick_y, color);
    for(y = start_pick_y; y < end_pick_y; y++)
        putpixel(start_pick_x, y, color);
    for(y = start_pick_y; y < end_pick_y; y++)
        putpixel(end_pick_x, y, color);
}

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("usage: %s <width> <height>\n", argv[0]);
        exit(1);
    }

    screen_width = atoi(argv[1]);
    screen_height = atoi(argv[2]);
    
    SDL_Init(SDL_INIT_VIDEO);

    sdl_set_video_mode(screen_width, screen_height, 0);

    origin = SDL_CreateRGBSurface(SDL_SWSURFACE, screen_width, screen_height, 32,
                                  screen->format->Rmask,
                                  screen->format->Gmask,
                                  screen->format->Bmask,
                                  screen->format->Amask);

    int done = 0;
    int dragging = 0;

    while(!done)
    {
        if(stale)
        {
            stale = 0;
            last_display = SDL_GetTicks();

            x0_inc = (re_max - re_min) / screen_width;
            y0_inc = (im_max - im_min) / screen_height;
            generate_mandel();
            SDL_BlitSurface(screen, NULL, origin, NULL);
            SDL_Flip(screen);
        }

        if(dragging)
        {
            SDL_BlitSurface(origin, NULL, screen, NULL);
            draw_box();
            SDL_Flip(screen);
        }
        
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.sym == SDLK_ESCAPE)
                    done = 1;
                else if(event.key.keysym.sym == SDLK_r) {
                    if(!dragging)
                        stale = 1;
                }
            } else if(event.type == SDL_MOUSEBUTTONDOWN) {
                if(event.button.button == SDL_BUTTON_LEFT) {
                    dragging = 1;
                    start_pick_x = end_pick_x = event.button.x;
                    start_pick_y = end_pick_y = event.button.y;
                } else if(event.button.button == SDL_BUTTON_RIGHT) {
                    update_coords();
                    stale = 1;
                }
            } else if(event.type == SDL_MOUSEBUTTONUP) {
                dragging = 0;
            } else if(event.type == SDL_MOUSEMOTION) {
                if(dragging == 1)
                {
                    end_pick_x = event.motion.x;
                    end_pick_y = event.motion.y;
                }
            }
        }
    }

    SDL_FreeSurface(origin);
    SDL_Quit();

    return 0;
}
