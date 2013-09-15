#include <cstdio>
#include <iostream>
#include <vector>
#include <thread>

#include <CL/cl.hpp>

#include "sdl_context.hpp"
#include "random.hpp"

using namespace std;

static bool stop_now = false;

class MandelSDLContext : public SDL_Context {
public:
    MandelSDLContext(double *data)
    : SDL_Context(1024, 768), data_(data)
    {}

private:
    double *data_;

protected:
    virtual void draw_screen() {
        for(int i = 0; i<1024*768; i++)
            pixels_[i] = data_[i];
        //pixels_[Random::U32MinMax(0, 480-1)*640 + Random::U32MinMax(0, 640-1)] = 0x000000ff;
    }
    
    virtual void handle_event() {
        if(event_.type == SDL_QUIT)
            stop_now = true;
    }
};

int main(int argc, char *argv[]) {
    vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if(all_platforms.size() == 0) {
        cout << "No platforms found!" << endl;
        exit(1);
    }

    cout << "Found " << all_platforms.size() << " platforms." << endl;

    cl::Platform default_platform = all_platforms[0];
    cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << endl;

    vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if(all_devices.size() == 0) {
        cout << "No devices found!" << endl;
        exit(1);
    }
    
    cout << "Found " << all_devices.size() << " devices." << endl;
    
    cl::Device default_device = all_devices[0];
    cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << endl;

    cl::Context context(default_device);
    cl::Program::Sources sources;

    FILE *f = fopen("test.cl", "r");
    fseek(f, 0L, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buffer = (char *)calloc(1, size+1);
    fread(buffer, size, 1, f);
    fclose(f);

    sources.push_back({buffer, size});

    cl::Program program(context, sources);
    if(program.build({default_device}) != CL_SUCCESS) {
        cout << "Error building program: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << endl;
        exit(1);
    }

    unsigned int width = 1024, height = 768;

    cl::Buffer buf_minx(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(double));
    cl::Buffer buf_maxx(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(double));
    cl::Buffer buf_miny(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(double));
    cl::Buffer buf_maxy(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(double));
    cl::Buffer buf_width(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(unsigned long));
    cl::Buffer buf_height(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(unsigned long));
    cl::Buffer buf_output(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                          sizeof(double) * width * height);

    cl::CommandQueue queue(context, default_device);

    double* map_minx = static_cast<double*>(queue.enqueueMapBuffer(buf_minx, CL_TRUE, CL_MAP_WRITE, 0, sizeof(double)));
    *map_minx = -1.5;
    double* map_maxx = static_cast<double*>(queue.enqueueMapBuffer(buf_maxx, CL_TRUE, CL_MAP_WRITE, 0, sizeof(double)));
    *map_maxx = 0.5;
    double* map_miny = static_cast<double*>(queue.enqueueMapBuffer(buf_miny, CL_TRUE, CL_MAP_WRITE, 0, sizeof(double)));
    *map_miny = -1;
    double* map_maxy = static_cast<double*>(queue.enqueueMapBuffer(buf_maxy, CL_TRUE, CL_MAP_WRITE, 0, sizeof(double)));
    *map_maxy = 1;
    unsigned int* map_width = static_cast<unsigned int*>(queue.enqueueMapBuffer(buf_width, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned int)));
    *map_width = width;
    unsigned int* map_height = static_cast<unsigned int*>(queue.enqueueMapBuffer(buf_height, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned int)));
    *map_height = height;

    double* map_output = static_cast<double*>(queue.enqueueMapBuffer(buf_output, CL_TRUE, CL_MAP_READ, 0, sizeof(double)*width*height));

    for(int i=0; i<width*height; i++)
        map_output[i] = 0;

    /*
    queue.enqueueUnmapMemObject(buf_minx, map_minx);
    queue.enqueueUnmapMemObject(buf_maxx, map_maxx);
    queue.enqueueUnmapMemObject(buf_miny, map_miny);
    queue.enqueueUnmapMemObject(buf_maxy, map_maxy);
    queue.enqueueUnmapMemObject(buf_width, map_width);
    queue.enqueueUnmapMemObject(buf_height, map_height);
    */

    cl::Kernel kernel(program, "mandelbrot");

    kernel.setArg<cl::Buffer>(0, buf_minx);
    kernel.setArg<cl::Buffer>(1, buf_maxx);
    kernel.setArg<cl::Buffer>(2, buf_miny);
    kernel.setArg<cl::Buffer>(3, buf_maxy);
    kernel.setArg<cl::Buffer>(4, buf_width);
    kernel.setArg<cl::Buffer>(5, buf_height);
    kernel.setArg<cl::Buffer>(6, buf_output);

    /*
    cl_int result = queue.enqueueNDRangeKernel(kernel,
                                               cl::NullRange,
                                               cl::NDRange(width, height),
                                               cl::NDRange(16, 16));
    */

    MandelSDLContext mandelSDLContext(map_output);

    for(int chunk_x=0; chunk_x<width; chunk_x += 64) {
        cl_int result = queue.enqueueNDRangeKernel(kernel,
                                                   cl::NDRange(chunk_x, 0),
                                                   cl::NDRange(64, height));
        
        if(result != CL_SUCCESS) {
            cout << "Error enqueuing kernel: " << result << endl;
            exit(1);
        }
        
        queue.finish();
        
        mandelSDLContext.draw();
        
        if(stop_now)
            break;
    }

    //queue.enqueueUnmapMemObject(buf_output, map_output);

    //queue.finish();

    //Random::SeedGood();

    //mandelSDLContext.finish();
        
    return 0;
}
