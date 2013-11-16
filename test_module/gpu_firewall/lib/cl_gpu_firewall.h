#ifndef __cl_gpu_firewall
#define __cl_gpu_firewall

#if defined (__APPLE__) || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>



cl_int init_gpu_firewall(cl_device_id* device_id, cl_context* context, cl_command_queue* command_queue, cl_program* program, cl_kernel* kernel);

cl_int run_gpu_firewall(cl_context* context, cl_command_queue* command_queue, cl_kernel* kernel, cl_mem* mem_obj, void* data, const unsigned int mat_size);



int do_work(int* packet_buff, int packet_buff_size);



#endif
