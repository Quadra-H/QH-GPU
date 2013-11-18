#include <stdio.h>
#include <string.h>

#include "bad_ip_address.h"
#include "cl_gpu_firewall.h"

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

	//CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE CL_QUEUE_PROFILING_ENABLE
	(*command_queue) = clCreateCommandQueue((*context), (*device_id), CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &res);
	ret |= res;

	(*kernel) = clCreateKernel((*program), "packet_matching", &res);
	ret |= res;

	return ret;
}

cl_int run_gpu_firewall(cl_context* context, cl_command_queue* command_queue, cl_kernel* kernel, cl_mem* packet_buff_mem_obj, cl_mem* packet_buff_mem_obj2, void* packet_buff, unsigned int packet_buff_size) {
	cl_int ret = 0;
	cl_int res = 0;

	//printf("packet_buff_size : %d \n",packet_buff_size );
	//packet_buff_size++;
	//packet_buff_size= 128;
	size_t globalThreads[2] = {32,32};

	//double size of wave front (128)
	//size_t localThreads[2] = {16, 8};
	size_t localThreads[2] = {16,8};

	(*packet_buff_mem_obj) = clCreateBuffer((*context), CL_MEM_READ_WRITE, 128*sizeof(int), NULL, &res);
	ret |= res;

	(*packet_buff_mem_obj2) = clCreateBuffer((*context), CL_MEM_READ_WRITE, 2048*sizeof(unsigned long), NULL, &res);
	ret |= res;

	ret |= clSetKernelArg((*kernel), 0, sizeof(cl_mem), packet_buff_mem_obj);
	ret |= clSetKernelArg((*kernel), 1, sizeof(cl_mem), packet_buff_mem_obj2);
	ret |= clSetKernelArg((*kernel), 2, sizeof(int), &packet_buff_size);

	//blocking_wirte FALSE
	ret |= clEnqueueWriteBuffer((*command_queue), (*packet_buff_mem_obj), CL_FALSE, 0, 128*sizeof(int), packet_buff, 0, NULL, NULL);
	ret |= clEnqueueWriteBuffer((*command_queue), (*packet_buff_mem_obj2), CL_FALSE, 0, 2048*sizeof(unsigned long), bad_ip_address, 0, NULL, NULL);
	ret |= clEnqueueNDRangeKernel((*command_queue), (*kernel), 2, NULL, globalThreads, localThreads, 0, NULL, NULL);
	ret |= clEnqueueReadBuffer((*command_queue), (*packet_buff_mem_obj), CL_FALSE, 0, 128*sizeof(int), packet_buff, 0, NULL, NULL);

	clFinish((*command_queue));
	clReleaseMemObject((*packet_buff_mem_obj));
	clReleaseMemObject((*packet_buff_mem_obj2));

	return ret;
}

