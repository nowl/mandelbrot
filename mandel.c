#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <SDL.h>
#include <gmp.h>

static SDL_Surface *screen, *origin;
static int screen_width, screen_height;
static mpf_t re_min;
static mpf_t re_max;
static mpf_t im_min;
static mpf_t im_max;
static mpf_t x0_inc, y0_inc;

static double re_min_d = -2.0;
static double re_max_d = 1.0;
static double im_min_d = -1.0;
static double im_max_d = 1.0;
static double x0_inc_d, y0_inc_d;

static int max_iters = 500;
static int stale = 1;
static int start_pick_x, start_pick_y, end_pick_x, end_pick_y;
static Uint32 *shared_data;
static float *status;
static int num_processes = 1;
static int smooth_coloring = 0;
static int multiple_precision = 0;

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

int smooth_color(double mag2, int iters)
{
    double smooth =  iters + 1 - log(log(sqrt(mag2)))/log(2);
                   
    double mod = 10.0f;
                   
    if(smooth < 0.167)
        return SDL_MapRGB(screen->format, 0, 0, smooth * mod);
    else if(smooth < 0.333)
        return SDL_MapRGB(screen->format, 0, smooth * mod, (1-smooth) * mod);
    else if(smooth < 0.5)
        return SDL_MapRGB(screen->format, smooth * mod, smooth * mod, (1-smooth) * mod);
    else if(smooth < 0.667)
        return SDL_MapRGB(screen->format, smooth * mod, smooth * mod, (1-smooth) * mod);
    else if(smooth < 0.833)
        return SDL_MapRGB(screen->format, smooth * mod, (1-smooth) * mod, (1-smooth) * mod);
    else
        return SDL_MapRGB(screen->format, (1-smooth) * mod, (1-smooth) * mod, (1-smooth) * mod);
}

int plain_color(int iters)
{
    float mod = 256.0f * 50;

    float norm_iters = (float)iters / max_iters;
                   
    if(norm_iters < 0.167)
        return SDL_MapRGB(screen->format, 0, 0, norm_iters * mod);
    else if(norm_iters < 0.333)
        return SDL_MapRGB(screen->format, 0, norm_iters * mod, (1-norm_iters) * mod);
    else if(norm_iters < 0.5)
        return SDL_MapRGB(screen->format, norm_iters * mod, norm_iters * mod, (1-norm_iters) * mod);
    else if(norm_iters < 0.667)
        return SDL_MapRGB(screen->format, norm_iters * mod, norm_iters * mod, (1-norm_iters) * mod);
    else if(norm_iters < 0.833)
        return SDL_MapRGB(screen->format, norm_iters * mod, (1-norm_iters) * mod, (1-norm_iters) * mod);
    else
        return SDL_MapRGB(screen->format, (1-norm_iters) * mod, (1-norm_iters) * mod, (1-norm_iters) * mod);
}

void run_child_generator(int process_num)
{
    int image_x, image_y;
    mpf_t x0, y0, x, y, xt, mag2, tmp, tmp2;

    mpf_init(x0);
    mpf_init(y0);
    mpf_init(x);
    mpf_init(y);
    mpf_init(xt);
    mpf_init(mag2);
    mpf_init(tmp);
    mpf_init(tmp2);

    /* use process_num and num_processes to determine how many columns
     * to skip and where to start the processing */
  
    mpf_set(x0, re_min);  
    mpf_mul_ui(tmp, x0_inc, process_num);
    mpf_add(x0, x0, tmp);
    mpf_mul_ui(x0_inc, x0_inc, num_processes);

    for(image_y = 0, image_x = process_num; image_x < screen_width; image_x += num_processes, image_y = 0)
    {
        mpf_set(y0, im_min);
        for(; image_y < screen_height; image_y++)
        {
            int iters = 0;
            mpf_set_d(x, 0.0);
            mpf_set_d(y, 0.0);
          
            while( iters < max_iters )
            {
                /* check for bailout */
                mpf_mul(mag2, x, x);
                mpf_mul(tmp, y, y);
                mpf_add(mag2, mag2, tmp);
                if( mpf_cmp_ui(mag2, 4) > 0 )
                    break;
              
                mpf_mul(tmp2, x, x);
                mpf_mul(tmp, y, y);
                mpf_sub(tmp, tmp2, tmp);
                mpf_add(tmp, tmp, x0);
                mpf_mul(tmp2, x, y);
                mpf_mul_ui(tmp2, tmp2, 2);
                mpf_add(y, tmp2, y0);
                mpf_set(x, tmp);

                iters++;
            }

            if(iters < max_iters)
            {
               
                Uint32 color;
               
                if(smooth_coloring)
                    color = smooth_color(mpf_get_d(mag2), iters);
                else
                    color = plain_color(iters);
               
                shared_data[image_y*screen_width + image_x] = color;
            }

            /* update status */
            status[process_num] = (float)image_x / screen_width;

            mpf_add(y0, y0, y0_inc);
        }

        mpf_add(x0, x0, x0_inc);
    }

    status[process_num] = 1.0f;
}

void run_child_generator_d(int process_num)
{
    int image_x, image_y;
    double x0, y0;

    /* use process_num and num_processes to determine how many columns
     * to skip and where to start the processing */
  
    x0 = re_min_d + x0_inc_d*process_num;
    for(image_y = 0, image_x = process_num; image_x < screen_width; image_x += num_processes, x0 += x0_inc_d*num_processes, image_y = 0)
    {
        for(y0 = im_min_d; image_y < screen_height; y0 += y0_inc_d, image_y++)
        {
            int iters = 0;
            double x = 0, y = 0;
            double mag2;
            while(iters < max_iters)
            {
                mag2 = x*x+y*y;
                if(mag2 >= 4)
                    break;

                double xt = x*x - y*y + x0;
                y = 2*x*y + y0;
                x = xt;
                iters++;
            }

            if(iters < max_iters)
            {
                Uint32 color;
               
                if(smooth_coloring)
                    color = smooth_color(mag2, iters);
                else
                    color = plain_color(iters);

                shared_data[image_y*screen_width + image_x] = color;
            }
        }

        /* update status */
        status[process_num] = (float)image_x / screen_width;
    }

    status[process_num] = 1.0f;
}

void update_coords()
{
    /* linearly interpolate to get coords from picked locations */
  
    mpf_t new_re_min, new_re_max;
    mpf_t new_im_min, new_im_max;

    mpf_init(new_re_min);
    mpf_init(new_re_max);
    mpf_init(new_im_min);
    mpf_init(new_im_max);
  
    mpf_t tmp;
  
    mpf_init(tmp);
  
    mpf_set_d(tmp, (float)start_pick_x / screen_width);
    mpf_sub(new_re_min, re_max, re_min);
    mpf_mul(new_re_min, new_re_min, tmp);
    mpf_add(new_re_min, new_re_min, re_min);

    mpf_set_d(tmp, (float)end_pick_x / screen_width);
    mpf_sub(new_re_max, re_max, re_min);
    mpf_mul(new_re_max, new_re_max, tmp);
    mpf_add(new_re_max, new_re_max, re_min);

    mpf_set_d(tmp, (float)start_pick_y / screen_height);
    mpf_sub(new_im_min, im_max, im_min);
    mpf_mul(new_im_min, new_im_min, tmp);
    mpf_add(new_im_min, new_im_min, im_min);

    mpf_set_d(tmp, (float)end_pick_y / screen_height);
    mpf_sub(new_im_max, im_max, im_min);
    mpf_mul(new_im_max, new_im_max, tmp);
    mpf_add(new_im_max, new_im_max, im_min);

    mpf_set(re_min, new_re_min);
    mpf_set(re_max, new_re_max);
    mpf_set(im_min, new_im_min);
    mpf_set(im_max, new_im_max);

    /* TODO: are these in the right order? */
    printf("new coords (%f, %f) to (%f, %f)\n",
           mpf_get_d(re_min),
           mpf_get_d(im_max),
           mpf_get_d(re_max),
           mpf_get_d(im_max));
}

void update_coords_d()
{
    /* linearly interpolate to get coords from picked locations */
  
    double new_re_min, new_re_max;
    double new_im_min, new_im_max;

    new_re_min = (re_max_d - re_min_d) * (double)start_pick_x / screen_width + re_min_d;
    new_re_max = (re_max_d - re_min_d) * (double)end_pick_x / screen_width + re_min_d;
    new_im_min = (im_max_d - im_min_d) * (double)start_pick_y / screen_height + im_min_d;
    new_im_max = (im_max_d - im_min_d) * (double)end_pick_y / screen_height + im_min_d;
   
    re_min_d = new_re_min;
    re_max_d = new_re_max;
    im_min_d = new_im_min;
    im_max_d = new_im_max;

    /* TODO: are these in the right order? */
    printf("new coords (%f, %f) to (%f, %f)\n", re_min_d, im_max_d, re_max_d, im_max_d);
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

void copy_data()
{
    int w, h;
    for(w=0; w<screen_width; w++)
        for(h=0; h<screen_height; h++)
            putpixel(w, h, shared_data[h*screen_width+w]);
}

void print_usage()
{
    printf("usage ./mandel [options] <width> <height>\n");
    printf("           -p             : turns on abritrarily deep precision\n");
    printf("           -s             : turns on smooth coloring\n");
    printf("           -m <iters>     : sets the maximum iterations for bailout\n");
    printf("           -n <processes> : sets the number of parallel processes to run\n");
}

int main(int argc, char *argv[])
{
    int c;

    while ((c = getopt (argc, argv, "psm:n:")) != -1)
        switch (c)
        {
        case 'p':
            multiple_precision = 1;
            break;
        case 's':
            smooth_coloring = 1;
            break;
        case 'm':
            max_iters = atoi(optarg);
            break;
        case 'n':
            num_processes = atoi(optarg);
            break;
        default:
            print_usage();
            exit(1);
        }
   
    if(argc - optind != 2)
    {
        print_usage();
        exit(1);
    }

    screen_width = atoi(argv[optind]);
    screen_height = atoi(argv[optind+1]);
   
    if(multiple_precision) {
        mpf_init_set_d(re_min, -2.0);
        mpf_init_set_d(re_max, 1.0);
        mpf_init_set_d(im_min, -1.0);
        mpf_init_set_d(im_max, 1.0);
        mpf_init(x0_inc);
        mpf_init(y0_inc);
    }
 
    SDL_Init(SDL_INIT_VIDEO);

    sdl_set_video_mode(screen_width, screen_height, 0);

    origin = SDL_CreateRGBSurface(SDL_SWSURFACE, screen_width, screen_height, 32,
                                  screen->format->Rmask,
                                  screen->format->Gmask,
                                  screen->format->Bmask,
                                  screen->format->Amask);

    pid_t *pids = malloc(sizeof(*pids) * num_processes);
    int shared_size = screen_width*screen_height*sizeof(Uint32) + sizeof(*status)*num_processes;
    void *mem = mmap(0, shared_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    shared_data = mem;
    status = mem + screen_height*screen_width*sizeof(Uint32);

    int done = 0;
    int dragging = 0;

    while(!done)
    {
        if(stale)
        {
            int start_time = SDL_GetTicks();

            stale = 0;
           
            if(multiple_precision) {
                mpf_sub(x0_inc, re_max, re_min);
                mpf_div_ui(x0_inc, x0_inc, screen_width);
                mpf_sub(y0_inc, im_max, im_min);
                mpf_div_ui(y0_inc, y0_inc, screen_height);
            } else {
                x0_inc_d = (re_max_d - re_min_d) / screen_width;
                y0_inc_d = (im_max_d - im_min_d) / screen_height;
            }

            /* clear data */
            memset(shared_data, 0, screen_height*screen_width*sizeof(Uint32));

            /* fork processes */          
            int i;
            for (i = 0; i < num_processes; i++) {
                pids[i] = fork();
                switch(pids[i]) {
                case -1:
                    printf("problem creating child process\n");
                    exit(1);
                case 0:
                    if(multiple_precision)
                        run_child_generator(i);
                    else
                        run_child_generator_d(i);
                    _exit(0);
                }
            }
          
            /* periodically update screen */
            float tot_status = 0;
            float last_update = 0;
            while(tot_status < 1.0)
            {
                for (tot_status = 0, i = 0; i < num_processes; i++)
                    tot_status += (float)status[i] / num_processes;

                /* update every 1% */
                if(tot_status - last_update > 0.01)
                {
                    last_update = tot_status;
                   
                    copy_data();
                    SDL_Flip(screen);
                }
            }
          
            /* wait for processes to finish */
            int stat;
            for (i = 0; i < num_processes; i++) {
                waitpid(pids[i], &stat, 0);
            }

            /* one more final update */
            copy_data();

            /* final update */
            SDL_BlitSurface(screen, NULL, origin, NULL);
            SDL_Flip(screen);

            printf("time to compute = %f sec\n", ((float)SDL_GetTicks() - start_time)/1000);
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
                    if(multiple_precision)
                        update_coords();
                    else
                        update_coords_d();
                    stale = 1;
                } else if(event.button.button == SDL_BUTTON_WHEELUP) {
                    int x_offset = screen_width * .75/2;
                    start_pick_x = event.button.x - x_offset;
                    end_pick_x = event.button.x + x_offset;
                    int y_offset = screen_height * .75/2;
                    start_pick_y = event.button.y - y_offset;
                    end_pick_y = event.button.y + y_offset;

                    if(multiple_precision)                        
                        update_coords();
                    else
                        update_coords_d();
                    stale = 1;
                } else if(event.button.button == SDL_BUTTON_WHEELDOWN) {
                    int x_offset = screen_width * 1.25/2;
                    start_pick_x = event.button.x - x_offset;
                    end_pick_x = event.button.x + x_offset;
                    int y_offset = screen_height * 1.25/2;
                    start_pick_y = event.button.y - y_offset;
                    end_pick_y = event.button.y + y_offset;

                    if(multiple_precision)                        
                        update_coords();
                    else
                        update_coords_d();
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

    free(pids);

    SDL_FreeSurface(origin);
    SDL_Quit();

    return 0;
}
