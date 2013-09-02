#include <stdio.h>
#include <string.h>

#include "cl_floyd_warshall_cco.h"

/********************************
 * init floyd warshall by gpu
 * load, build program
 * create buffer
 ********************************/
cl_int init_floyd_warshall_gpu(cl_device_id* device_ids, cl_context* context, cl_command_queue* command_queues, cl_program* program, cl_kernel* kernels) {
	cl_int ret = 0;
	cl_int res = 0;
	int i;

	const int MAX_SOURCE_SIZE = 0x100000;
	char* source_str;
	size_t source_size;

	// Load the kernel source code into the array source_str
	FILE *fp;
	fp = fopen("FloydWarshall_Kernels_cco.cl", "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		return 1;
	}

	source_str = (char*) malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);

	fclose(fp);

	// Create a program from the kernel source
	(*program) = clCreateProgramWithSource((*context), /*num of file*/1, (const char **) &(source_str), (const size_t *) &(source_size), &res);
	ret |= res;
	// Build the program
	ret |= clBuildProgram((*program), /*num of device*/1, device_ids, /*option*/NULL, /*pfn_notify?*/NULL, NULL);

	for(i = 0 ; i < 4 ; i++) {
		//CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE CL_QUEUE_PROFILING_ENABLE
		command_queues[i] = clCreateCommandQueue((*context), device_ids[0], CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &res);
		ret |= res;

		kernels[i] = clCreateKernel((*program), "floydWarshallPass", &res);
		ret |= res;
	}

	return ret;
}

cl_int run_floyd_warshall_gpu(cl_context* context, cl_command_queue* command_queues, cl_kernel* kernels, cl_mem* mem_objs, void* data, const unsigned int mat_size, int index) {
	cl_int ret = 0;
	cl_int res = 0;
	int i;

	size_t globalThreads[2] = {mat_size, mat_size};
	//double size of wave front (128)
	size_t localThreads[2] = {16, 8};

	int fs, fe;

	//단일 mem obj를 글로벌로 잡거나
	//4 mem obj를 글로벌로 잡을 경우
	//gpu - dev mem 통신이 병목이 될 수 있음
	//but 각 커널이 모든 데이터에 접근해야되.... 멸망이다......
	//todo : set mem local.......
	mem_objs[0] = clCreateBuffer((*context), CL_MEM_READ_WRITE, mat_size*mat_size*sizeof(int), NULL, &res);
	ret |= res;

	ret |= clSetKernelArg(kernels[index], 0, sizeof(cl_mem), mem_objs[0]);
	ret |= clSetKernelArg(kernels[index], 1, sizeof(int), &mat_size);

	//blocking_wirte FALSE
	ret |= clEnqueueWriteBuffer(command_queues[index], mem_objs[0], CL_FALSE, 0, mat_size*mat_size*sizeof(int), data, 0, NULL, NULL);

	fs = index * mat_size / 4;
	fs = (index + 1) * mat_size / 4;

	for(i = fs ; i < fe ; i++ ) {
		ret |= clSetKernelArg(kernels[index], 2, sizeof(int), &i);
		ret |= clEnqueueNDRangeKernel(command_queues[index], kernels[index], 2, NULL, globalThreads, localThreads, 0, NULL, NULL);

		//don`t have to?
		ret |= clFlush(command_queues[index]);
	}

	ret |= clEnqueueReadBuffer(command_queues[index], mem_objs[0], CL_FALSE, 0, mat_size*mat_size*sizeof(int), data, 0, NULL, NULL);

	return ret;
}

