#include <stdio.h>
#include <string.h>

#include "cl_gpu_firewall.h"

#define IP_BASE (0x11111111)

/********************************
 * init gpu firewall by gpu
 * load, build program
 * create buffer
 ********************************/
cl_int init_gpu_firewall(cl_device_id* device_id, cl_context* context, cl_command_queue* command_queue, cl_program* program, cl_kernel* kernel) {
	cl_int ret = 0;
	cl_int res = 0;

	const int MAX_SOURCE_SIZE = 0x100000;
	char* source_str;
	size_t source_size;

	// Load the kernel source code into the array source_str
	FILE *fp;
	fp = fopen("gpu_firewall_Kernels.cl", "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		return 1;
	}

	source_str = (char*) malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);

	fclose(fp);

	// Create a program from the kernel source
	(*program) = clCreateProgramWithSource((*context), 1, (const char **) &(source_str), (const size_t *) &(source_size), &res);
	ret |= res;
	// Build the program
	ret |= clBuildProgram((*program), 1, device_id, NULL, NULL, NULL);
	if(ret != CL_SUCCESS) printf("bp err");


	//CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE CL_QUEUE_PROFILING_ENABLE
	(*command_queue) = clCreateCommandQueue((*context), (*device_id), CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &res);
	ret |= res;

	(*kernel) = clCreateKernel((*program), "packet_matching", &res);
	ret |= res;

	return ret;
}

cl_int run_gpu_firewall(cl_context* context, cl_command_queue* command_queue, cl_kernel* kernel, cl_mem* packet_buff_mem_obj, void* packet_buff,unsigned int packet_buff_size) {
	int ip_base;
	ip_base = IP_BASE;

	cl_int ret = 0;
	cl_int res = 0;




	printf("packet_buff_size : %d \n",packet_buff_size );
	//packet_buff_size++;
	//packet_buff_size= 128;
	size_t globalThreads[1] = {packet_buff_size};

	//double size of wave front (128)
	//size_t localThreads[2] = {16, 8};
	size_t localThreads[1] = {64};

	(*packet_buff_mem_obj) = clCreateBuffer((*context), CL_MEM_READ_WRITE, packet_buff_size*sizeof(int), NULL, &res);
	ret |= res;



    if(ret == CL_INVALID_CONTEXT)
    	printf("if context is not a valid context..\n");
    else if(ret == CL_INVALID_VALUE)
        	printf("if values specified in flags are not valid.\n");
    else if(ret == CL_INVALID_BUFFER_SIZE)
            	printf("if context is not a valid context..1\n");
    else if(ret == CL_INVALID_HOST_PTR)
            	printf("if context is not a valid context..2\n");
    else if(ret == CL_MEM_OBJECT_ALLOCATION_FAILURE)
            	printf("if context is not a valid context..3\n");
    else if(ret == CL_OUT_OF_HOST_MEMORY)
            	printf("if context is not a valid context..4\n");



//     if size is 0 or is greater than CL_DEVICE_MAX_MEM_ALLOC_SIZE value specified in table of OpenCL Device Queries for clGetDeviceInfo for all devices in context.
//     if host_ptr is NULL and CL_MEM_USE_HOST_PTR or CL_MEM_COPY_HOST_PTR are set in flags or if host_ptr is not NULL but CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR are not set in flags.
//     if there is a failure to allocate memory for buffer object.
//     if there is a failure to allocate resources required by the OpenCL implementation on the host.




	assert(ret == CL_SUCCESS);

	ret |= clSetKernelArg((*kernel), 0, sizeof(cl_mem), packet_buff_mem_obj);
	assert(ret == CL_SUCCESS);
	ret |= clSetKernelArg((*kernel), 1, sizeof(int), &packet_buff_size);
	assert(ret == CL_SUCCESS);

	ret |= clSetKernelArg((*kernel), 2, sizeof(int), &ip_base);
	assert(ret == CL_SUCCESS);

	//blocking_wirte FALSE
	ret |= clEnqueueWriteBuffer((*command_queue), (*packet_buff_mem_obj), CL_FALSE, 0, packet_buff_size*sizeof(int), packet_buff, 0, NULL, NULL);
	assert(ret == CL_SUCCESS);

	ret |= clEnqueueNDRangeKernel((*command_queue), (*kernel), 1, NULL, globalThreads, localThreads, 0, NULL, NULL);
	assert(ret == CL_SUCCESS);

	ret |= clEnqueueReadBuffer((*command_queue), (*packet_buff_mem_obj), CL_FALSE, 0, packet_buff_size*sizeof(int), packet_buff, 0, NULL, NULL);
	assert(ret == CL_SUCCESS);

	clFinish((*command_queue));
	clReleaseMemObject((*packet_buff_mem_obj));

	return ret;
}

