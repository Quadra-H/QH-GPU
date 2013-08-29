#include "cl_bfs.h"
#include <stdio.h>
#define MAX_SOURCE_SIZE (0x100000)
#define WORK_DIM 2	//work-items dimensions

typedef enum { false, true} bool;

const char* kernel_names[2] = {"BFS_1", "BFS_2"};

int device_id_inused = 0;

cl_int resultCL = CL_SUCCESS;


void _clInit(cl_context ctx,cl_device_id* devices)
{
	int DEVICE_ID_INUSED = 0;

	oclHandles.context = ctx;
	oclHandles.devices = devices;
	oclHandles.queue = NULL;
	oclHandles.program = NULL;

	cl_uint deviceListSize;

	cl_uint numPlatforms;
	cl_platform_id targetPlatform = NULL;



	oclHandles.queue = clCreateCommandQueue(oclHandles.context,
											oclHandles.devices[DEVICE_ID_INUSED],
											0,
											&resultCL);

	if ((resultCL != CL_SUCCESS) || (oclHandles.queue == NULL))
		printf("InitCL()::Creating Command Queue. (clCreateCommandQueue)\n");

	FILE * fp;
	fp = fopen("bfs_kernels.cl", "r");
	if(!fp){
		printf("Failed kernel_file\n");
		exit(EXIT_FAILURE);
	}

	char* source = (char*)malloc(MAX_SOURCE_SIZE);
	size_t sourceSize = fread(source, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	oclHandles.program = clCreateProgramWithSource(oclHandles.context,
													1,
													(const char**)&source,
													&sourceSize,
													&resultCL);

	if ((resultCL != CL_SUCCESS) || (oclHandles.program == NULL))
	    printf("InitCL()::Error: Loading Binary into cl_program. (clCreateProgramWithBinary\n");

	//options += " -cl-nv-opt-level=3";
	resultCL = clBuildProgram(oclHandles.program, deviceListSize, oclHandles.devices, "-cl-nv-verbose", NULL,  NULL);

	if ((resultCL != CL_SUCCESS) || (oclHandles.program == NULL))
	{
		printf("InitCL()::Error: In clBuildProgram\n");

		size_t length;
		resultCL = clGetProgramBuildInfo(oclHandles.program,
										oclHandles.devices[DEVICE_ID_INUSED],
										CL_PROGRAM_BUILD_LOG,
										0,
										NULL,
										&length);
		if(resultCL != CL_SUCCESS)
			printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");

		char* buffer = (char*)malloc(length);
		resultCL = clGetProgramBuildInfo(oclHandles.program,
										oclHandles.devices[DEVICE_ID_INUSED],
										CL_PROGRAM_BUILD_LOG,
										length,
										buffer,
										NULL);
		if(resultCL != CL_SUCCESS)
			printf("InitCL()::Error: Getting Program build info(clGetProgramBuildInfo)\n");

		free(buffer);

		printf("InitCL()::Error: Building Program (clBuildProgram)\n");
	}

	int nKernel;
	for (nKernel = 0; nKernel < 2; nKernel++)
	{
		/* get a kernel object handle for a kernel with the given name */
		cl_kernel kernel = clCreateKernel(oclHandles.program,
											kernel_names[nKernel],
											&resultCL);

		if ((resultCL != CL_SUCCESS) || (kernel == NULL))
		{
			printf("ERROR : clCreateKernel\n");
		}

		oclHandles.kernels[nKernel] = kernel;
	}
}

void _clRelease()
{
	bool errorFlag = false;

	int nKernel;
	for(nKernel = 0; nKernel < 2; nKernel++)
	{
		if(oclHandles.kernels[nKernel] != NULL)
		{
			cl_int resultCL = clReleaseKernel(oclHandles.kernels[nKernel]);
			if(resultCL != CL_SUCCESS){
				printf("ERROR clRealseKernel");
				exit(EXIT_FAILURE);
			}
		}

		oclHandles.kernels[nKernel] = NULL;
	}


	if(oclHandles.program != NULL)
	{
		cl_int resultCL = clReleaseProgram(oclHandles.program);
		if(resultCL != CL_SUCCESS){
			printf("ERROR clReleaseProgram\n");
			exit(EXIT_FAILURE);
		}
		oclHandles.program = NULL;
	}

	if(oclHandles.queue != NULL)
	{
		 cl_int resultCL = clReleaseContext(oclHandles.context);
		if (resultCL != CL_SUCCESS)
		{
			printf("ERROR clReleaseProgram\n");
						exit(EXIT_FAILURE);
		}
		oclHandles.context = NULL;
    }

    if (errorFlag) printf("ReleaseCL()::Error encountered.");
}



cl_mem _clCreateAndCpyMem(int size, void * h_mem_source)
{
	cl_mem d_mem;
	d_mem = clCreateBuffer(oclHandles.context,	CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, size, h_mem_source, &oclHandles.cl_status);

	if(oclHandles.cl_status != CL_SUCCESS)
		printf("excpetion in _clCreateAndCpyMem()");

	return d_mem;
}

cl_mem _clMallocRW(int size, void * h_mem_ptr)
{
 	cl_mem d_mem;
	d_mem = clCreateBuffer(oclHandles.context, CL_MEM_READ_WRITE, size, h_mem_ptr, &oclHandles.cl_status);

	if(oclHandles.cl_status != CL_SUCCESS)
		printf("excpetion in _clMallocRW");

	return d_mem;
}


































cl_mem _clMalloc(int size, void * h_mem_ptr)
{
 	cl_mem d_mem;
	d_mem = clCreateBuffer(oclHandles.context, CL_MEM_READ_ONLY, size, h_mem_ptr, &oclHandles.cl_status);

	if(oclHandles.cl_status != CL_SUCCESS)
		printf("excpetion in _clMalloc");

	return d_mem;
}

void _clAsyncMemcpyH2D(cl_mem d_mem, int size, const void* h_mem_ptr)
{

}

void _clMemcpyH2D(cl_mem d_mem, int size, const void *h_mem_ptr)
{
	oclHandles.cl_status = clEnqueueWriteBuffer(oclHandles.queue, d_mem, CL_TRUE, 0, size, h_mem_ptr, 0, NULL, NULL);

	if(oclHandles.cl_status != CL_SUCCESS)
		printf("excpetion in _clMemcpyH2D");

}


cl_mem _clCreateAndCpyPinnedMem(int size, float* h_mem_source)
{

	cl_mem d_mem, d_mem_pinned;
	unsigned char* h_mem_pinned = NULL;
	d_mem_pinned = clCreateBuffer(oclHandles.context,	CL_MEM_READ_ONLY|CL_MEM_ALLOC_HOST_PTR,  \
									size, NULL, &oclHandles.cl_status);

	h_mem_pinned = (unsigned char*)clEnqueueMapBuffer(oclHandles.queue, d_mem_pinned, CL_FALSE,  \
										CL_MAP_WRITE, 0, size, NULL, NULL,  \
										NULL,  &oclHandles.cl_status);

	memcpy(h_mem_pinned, h_mem_source, size);

	d_mem = clCreateBuffer(oclHandles.context,	CL_MEM_READ_ONLY,  \
									size, NULL, &oclHandles.cl_status);
	clEnqueueUnmapMemObject(oclHandles.queue, d_mem_pinned, (void*)h_mem_pinned, 0, NULL, NULL);
//	if(accMode == DIRECT)
//	{
		h_mem_pinned = (unsigned char*)clEnqueueMapBuffer(oclHandles.queue, d_mem_pinned, CL_FALSE,  \
										CL_MAP_WRITE, 0, size, NULL, NULL,  \
										NULL,  &oclHandles.cl_status);
		// DIRECT:  API access to device buffer
		//for(int i = 0; i < MEMCOPY_ITERATIONS; i++)
		oclHandles.cl_status = clEnqueueWriteBuffer(oclHandles.queue,d_mem, 	\
									CL_FALSE, 0, size, h_mem_pinned,  \
									0, NULL, NULL);
		_clFinish();

		#ifdef ERRMSG
		if(oclHandles.cl_status != CL_SUCCESS)
			printf("excpetion in _clCreateAndCpyMem() -> clEnqueueWriteBuffer");
		#endif
//	}
//	else
//	{

		// MAPPED: mapped pointers to device buffer and conventional pointer access
//		void* dm_idata = clEnqueueMapBuffer(oclHandles.queue, d_mem, CL_TRUE, CL_MAP_WRITE, 0, size, 0, NULL, NULL, &oclHandles.cl_status);
//
//
//		memcpy(dm_idata, h_mem_pinned, size);
//
//		clEnqueueUnmapMemObject(oclHandles.queue, d_mem, dm_idata, 0, NULL, NULL);
	//}

	return d_mem;
}

cl_mem _clCreateAndCpyPagedMem(int size, unsigned char* h_mem_source)
{
	cl_mem d_mem;
	float * h_mem_pagealbe = NULL;

	d_mem = clCreateBuffer(oclHandles.context, CL_MEM_READ_WRITE, size, NULL, &oclHandles.cl_status);

	h_mem_pagealbe = (float*)malloc(size);

	h_mem_pagealbe = (float *)clEnqueueMapBuffer(oclHandles.queue, d_mem, CL_TRUE,  \
										CL_MAP_WRITE, 0, size, NULL, NULL,  \
										NULL,  &oclHandles.cl_status);


	memcpy(h_mem_pagealbe, h_mem_source, size);

	oclHandles.cl_status = clEnqueueWriteBuffer(oclHandles.queue, d_mem, CL_FALSE, 0, size, h_mem_pagealbe, 0, NULL, NULL);
	_clFinish();

	return d_mem;
}

cl_mem _clMallocWO(int size)
{
	cl_mem d_mem;
	d_mem = clCreateBuffer(oclHandles.context, CL_MEM_WRITE_ONLY, size, 0, &oclHandles.cl_status);

	if(oclHandles.cl_status != CL_SUCCESS)
		printf("excpetion in _clCreateMem()");

	return d_mem;
}

//--------------------------------------------------------
//transfer data from device to host
void _clMemcpyD2H(cl_mem d_mem, int size, void * h_mem)
{
	oclHandles.cl_status = clEnqueueReadBuffer(oclHandles.queue, d_mem, CL_TRUE, 0, size, h_mem, 0,0,0);

	if(oclHandles.cl_status != CL_SUCCESS)
		printf("clMemecpyD2H ERROR\n");
}

void _clSetArgs_1(int kernel_id, int arg_idx, void * d_mem, int size)
{
	oclHandles.cl_status = clSetKernelArg(oclHandles.kernels[kernel_id], arg_idx, size, d_mem);
	if(oclHandles.cl_status != CL_SUCCESS)
	{
		printf("ERROR : clSetKernelArg in _clSetArgs_1\n");
		exit(EXIT_FAILURE);
	}
}

void _clSetArgs(int kernel_id, int arg_idx, void * d_mem)
{
	oclHandles.cl_status = clSetKernelArg(oclHandles.kernels[kernel_id], arg_idx, sizeof(d_mem), &d_mem);
	if(oclHandles.cl_status != CL_SUCCESS)
	{
		printf("ERROR clSetKernelArg\n");
		exit(EXIT_FAILURE);
	}
}

void _clInvokeKernel(int kernel_id, int work_items, int work_group_size)
{
	cl_uint work_dim = WORK_DIM;
	cl_event e[1];
	if(work_items%work_group_size != 0)
		  work_items = work_items + (work_group_size-(work_items%work_group_size));
	size_t local_work_size[] = {work_group_size, 1};
	size_t global_work_size[] = {work_items, 1};
	oclHandles.cl_status = clEnqueueNDRangeKernel(oclHandles.queue, oclHandles.kernels[kernel_id], work_dim, 0, \
												global_work_size, local_work_size, 0 , 0, &(e[0]) );
	if(oclHandles.cl_status != CL_SUCCESS)
	{
		printf("ERROR : clEnqueueNDRangeKernel in _clInvokeKernel\n");
		exit(EXIT_FAILURE);
	}
}

void _clInvokeKernel2D(int kernel_id, int range_x, int range_y, int group_x, int group_y)
{
	cl_uint work_dim = WORK_DIM;
	size_t local_work_size[] = {group_x, group_y};
	size_t global_work_size[] = {range_x, range_y};
	cl_event e[1];
	/*if(work_items%work_group_size != 0)	//process situations that work_items cannot be divided by work_group_size
	  work_items = work_items + (work_group_size-(work_items%work_group_size));*/
	oclHandles.cl_status = clEnqueueNDRangeKernel(oclHandles.queue, oclHandles.kernels[kernel_id], work_dim, 0, \
											global_work_size, local_work_size, 0 , 0, &(e[0]) );
	printf("ERROR : clEnqueueNDRangeKernel in _clInvokeKernel2D\n");
}


void _clFinish()
{
	oclHandles.cl_status = clFinish(oclHandles.queue);
	if(oclHandles.cl_status != CL_SUCCESS)
	{
		printf("ERROR : clFinish in _clFinish\n");
		exit(EXIT_FAILURE);
	}
}

void _clFree(cl_mem ob)
{
	if(ob!=NULL)
		oclHandles.cl_status = clReleaseMemObject(ob);
	if(oclHandles.cl_status != CL_SUCCESS)
	{
		printf("_clReleasMemObject in _clFree()");
	}
}


//free pinned memory
void _clFreePinnedMem(void * mem_h, int idxQ)
{
	if(mem_h)
	{
		///oclHandles.cl_status = clEnqueueUnmapMemObject(oclHandles.queue[idxQ], oclHandles.pinned_mem, (void*)mem_h, 0, NULL, NULL);
		if(oclHandles.cl_status != CL_SUCCESS)
		{
			printf("exception in _clFreeHost -> clEnqueueUnmapMemObject(in)");
		}
	}
}



