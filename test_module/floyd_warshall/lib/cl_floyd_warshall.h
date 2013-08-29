#ifndef __cl_floyd_warshall
#define __cl_floyd_warshall


#if defined (__APPLE__) || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

cl_int init_floyd_warshall_gpu(cl_device_id* device_id, cl_context* context, cl_command_queue* command_queue, cl_program* program, cl_kernel* kernel);

cl_int run_floyd_warshall_gpu(cl_context* context, cl_command_queue* command_queue, cl_kernel* kernel, cl_mem* mem_obj, void* data, const unsigned int mat_size);

#endif
