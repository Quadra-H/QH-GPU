
/* This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../../qhgpu/qhgpu.h"
#include "../../../qhgpu/connector.h"
#include "cl_floyd_warshall.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


struct floyd_warshall_data {
	cl_context context;
	cl_device_id device_id;
	cl_command_queue command_queue;
	cl_program program;
	cl_kernel kernel;
	cl_mem mem_obj;
};

struct floyd_warshall_data fw_data;

// Default Process
int floyd_warshall_cs(struct qhgpu_service_request *sr) {
	printf("[libsrv_default] Info: floyd_warshall_cs\n");
	return 0;
}


int floyd_warshall_launch(struct qhgpu_service_request *sr)
{
	cl_int res;
	int* mmap_data;
	unsigned int mmap_size;

	double data_size;
	unsigned int mat_size;

	printf("[libsrv_floyd_warshall]floyd_warshall launch\n");

	mmap_data = sr->mmap_addr;
	mmap_size = sr->mmap_size;
	printf("[libsrv_floyd_warshall] mmap_addr[%p], mmap_size[%x]\n", mmap_data, mmap_size);

	data_size = mmap_size;
	mat_size = sqrt(data_size);

	res = run_floyd_warshall_gpu(&(fw_data.context), &(fw_data.command_queue), &(fw_data.kernel), &(fw_data.mem_obj), mmap_data, mat_size);
	if( res != CL_SUCCESS ) {
		printf("init_openCL error.\n");
		return 1;
	}

	clFinish(fw_data.command_queue);
	clReleaseMemObject(fw_data.mem_obj);

	return 0;
}

int floyd_warshall_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_floyd_warshall] Info: floyd_warshall_post\n");
	return 0;
}

static struct qhgpu_service floyd_warshall_srv;

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*), cl_context context, cl_device_id* device_id) {
	cl_int ret;

	printf("[libsrv_floyd_warshall] Info: init libsrv floyd warshall.\n");

	fw_data.context = context;
	fw_data.device_id = device_id;

	ret = init_floyd_warshall_gpu(device_id, &context, &(fw_data.command_queue), &(fw_data.program), &(fw_data.kernel));
	if( ret != CL_SUCCESS) {
		printf("init_floyd_warshall_gpu error.\n");
		return 1;
	}

	sprintf(floyd_warshall_srv.name, "floyd_warshall_service");
	floyd_warshall_srv.compute_size = floyd_warshall_cs;
	floyd_warshall_srv.launch = floyd_warshall_launch;
	floyd_warshall_srv.post = floyd_warshall_post;

	return reg_srv(&floyd_warshall_srv, lh);
}

int finit_service(void *lh, int (*unreg_srv)(const char*)) {
	printf("[libsrv_floyd_warshall] Info: finit libsrv floyd warshall.\n");

	clFinish(fw_data.command_queue);
	clReleaseKernel(fw_data.kernel);
	clReleaseProgram(fw_data.program);
	clReleaseCommandQueue(fw_data.command_queue);

	return unreg_srv(floyd_warshall_srv.name);
}
