#ifndef _cl_bfs
#define _cl_bfs

#if defined (__APPLE__) || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif


#define MAX_SOURCE_SIZE (0x100000)
#define WORK_DIM 2	//work-items dimensions



struct oclHandlesStruct
{
	cl_context context;
	cl_device_id *devices;
	cl_command_queue *queues;
	cl_command_queue queue;
	cl_program program;
	cl_int cl_status;
	cl_kernel kernels[2];

};

struct oclHandlesStruct oclHandles;

void _clInit(cl_context ctx,cl_device_id* devices);
void _clRelease();
cl_mem _clCreateAndCpyMem(int size, void * h_mem_source);
cl_mem _clMallocRW(int size, void * h_mem_ptr);
cl_mem _clMalloc(int size, void * h_mem_ptr);
void _clMemcpyH2D(cl_mem d_mem, int size, const void *h_mem_ptr);
cl_mem _clCreateAndCpyPinnedMem(int size, float* h_mem_source);
cl_mem _clCreateAndCpyPagedMem(int size, unsigned char* h_mem_source);
cl_mem _clMallocWO(int size);
void _clMemcpyD2H(cl_mem d_mem, int size, void * h_mem);
void _clSetArgs_1(int kernel_id, int arg_idx, void * d_mem, int size);
void _clSetArgs(int kernel_id, int arg_idx, void * d_mem);
void _clInvokeKernel(int kernel_id, int work_items, int work_group_size);
void _clInvokeKernel2D(int kernel_id, int range_x, int range_y, int group_x, int group_y);
void _clFinish();
void _clFree(cl_mem ob);
void _clFreePinnedMem(void * mem_h, int idxQ);

#endif


