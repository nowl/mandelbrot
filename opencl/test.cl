#define BAILOUT        4
#define ITERATIONS   10000

__kernel void mandelbrot(__global double *g_minx,
                         __global double *g_maxx,
                         __global double *g_miny,
                         __global double *g_maxy,
                         __global unsigned int *g_width,
                         __global unsigned int *g_height,
                         __global double *g_output)
{
    __private unsigned int i = get_global_id(0);
    __private unsigned int j = get_global_id(1);
    __private double minx = *g_minx;
    __private double maxx = *g_maxx;
    __private double miny = *g_miny;
    __private double maxy = *g_maxy;
    __private unsigned int width = *g_width;
    __private unsigned int height = *g_height;
    
    double2 z = {0, 0};
    double2 c = {(maxx - minx) * i/width + minx, (maxy - miny) * j/height + miny};
    double zl_sqr;
    unsigned int iter;
    double tmp;
    
    for (iter=0; iter<ITERATIONS; iter++) {
        zl_sqr = z.x*z.x+z.y*z.y;
        if(zl_sqr > BAILOUT)
            break;
        tmp = z.x*z.x - z.y*z.y + c.x;
        z.y = 2*z.x*z.y + c.y;
        z.x = tmp;
    }

    write_mem_fence(CLK_GLOBAL_MEM_FENCE);
    
    g_output[j * width + i] = iter;
}
