#include <stdio.h>
#include <string.h>

#include "cl_floyd_warshall.h"

/********************************
 * init openCL platform, device, context
 ********************************/
/*cl_int init_openCL(cl_platform_id* platform_id, cl_device_id* device_id, cl_context* context) {
	cl_int ret = 0;
	cl_int res = 0;

	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;

	// Get platform and device information
	(*platform_id) = NULL;
	(*device_id) = NULL;

	ret |= clGetPlatformIDs(1, platform_id, &ret_num_platforms);
	ret |= clGetDeviceIDs((*platform_id), CL_DEVICE_TYPE_GPU, 1, device_id, &ret_num_devices);

	// Create an OpenCL context
	(*context) = clCreateContext(NULL, 1, device_id, NULL, NULL, &res);
	ret |= res;

	return ret;
}*/

/********************************
 * init floyd warshall by gpu
 * load, build program
 * create buffer
 ********************************/
cl_int init_floyd_warshall_gpu(cl_device_id* device_id, cl_context* context, cl_command_queue* command_queue, cl_program* program, cl_kernel* kernel) {
	cl_int ret = 0;
	cl_int res = 0;

	const int MAX_SOURCE_SIZE = 0x100000;
	char* source_str;
	size_t source_size;

	// Load the kernel source code into the array source_str
	FILE *fp;
	fp = fopen("FloydWarshall_Kernels.cl", "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		return 1;
	}

	source_str = (char*) malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);

	fclose(fp);

	// Create a program from the kerne16l source
	(*program) = clCreateProgramWithSource((*context), /*num of file*/1, (const char **) &(source_str), (const size_t *) &(source_size), &res);
	ret |= res;
	// Build the program
	ret |= clBuildProgram((*program), /*num of device*/1, device_id, /*option*/NULL, /*pfn_notify?*/NULL, NULL);

	//CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE CL_QUEUE_PROFILING_ENABLE
	(*command_queue) = clCreateCommandQueue((*context), (*device_id), CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &res);
	ret |= res;

	(*kernel) = clCreateKernel((*program), "floydWarshallPass", &res);
	ret |= res;

	return ret;
}

cl_int run_floyd_warshall_gpu(cl_context* context, cl_command_queue* command_queue, cl_kernel* kernel, cl_mem* mem_obj, void* data, const unsigned int mat_size) {
	int i;

	cl_int ret = 0;
	cl_int res = 0;

	size_t globalThreads[2] = {mat_size, mat_size};
	//double size of wave front (128)
	size_t localThreads[2] = {16, 8};

	(*mem_obj) = clCreateBuffer((*context), CL_MEM_READ_WRITE, mat_size*mat_size*sizeof(int), NULL, &res);
	ret |= res;

	ret |= clSetKernelArg((*kernel), 0, sizeof(cl_mem), mem_obj);
	ret |= clSetKernelArg((*kernel), 1, sizeof(int), &mat_size);

	//blocking_wirte FALSE
	ret |= clEnqueueWriteBuffer((*command_queue), (*mem_obj), CL_FALSE, 0, mat_size*mat_size*sizeof(int), data, 0, NULL, NULL);

	for(i = 0 ; i < mat_size ; i++ ) {
		ret |= clSetKernelArg((*kernel), 2, sizeof(int), &i);
		ret |= clEnqueueNDRangeKernel((*command_queue), (*kernel), 2, NULL, globalThreads, localThreads, 0, NULL, NULL);

		//don`t have to?
		//ret |= clFlush((*command_queue));
	}

	ret |= clEnqueueReadBuffer((*command_queue), (*mem_obj), CL_FALSE, 0, mat_size*mat_size*sizeof(int), data, 0, NULL, NULL);

	return ret;
}

