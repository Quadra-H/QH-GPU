
/* This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */
#include <iostream>
#include <fstream>
#include <sstream>

extern "C" {
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include "../../qhgpu/qhgpu.h"
	#include "../../qhgpu/connector.h"
}


#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


///
//  Constants
//
const int ARRAY_SIZE = 10;


cl_context context = 0;
cl_command_queue commandQueue = 0;
cl_program program = 0;
cl_device_id device = 0;
cl_kernel kernel = 0;
cl_mem memObjects[3] = { 0, 0, 0 };
cl_int errNum;
int result[ARRAY_SIZE];



///
//  Create an OpenCL context on the first available platform using
//  either a GPU or CPU depending on what is available.
//
extern "C" cl_context CreateContext()
{
	cl_int errNum;
	cl_uint numPlatforms;
	cl_platform_id firstPlatformId;
	cl_context context = NULL;

	// First, select an OpenCL platform to run on.  For this example, we
	// simply choose the first available platform.  Normally, you would
	// query for all available platforms and select the most appropriate one.
	errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
	if (errNum != CL_SUCCESS || numPlatforms <= 0)
	{
		std::cerr << "Failed to find any OpenCL platforms." << std::endl;
		return NULL;
	}

	// Next, create an OpenCL context on the platform.  Attempt to
	// create a GPU-based context, and if that fails, try to create
	// a CPU-based context.
	cl_context_properties contextProperties[] =
	{
			CL_CONTEXT_PLATFORM,
			(cl_context_properties)firstPlatformId,
			0
	};
	context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
			NULL, NULL, &errNum);
	if (errNum != CL_SUCCESS)
	{
		std::cout << "Could not create GPU context, trying CPU..." << std::endl;
		context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
				NULL, NULL, &errNum);
		if (errNum != CL_SUCCESS)
		{
			std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
			return NULL;
		}
	}
	return context;
}



//  Create a command queue on the first device available on the
//  context
//
extern "C" cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device)
{
	cl_int errNum;
	cl_device_id *devices;
	cl_command_queue commandQueue = NULL;
	size_t deviceBufferSize = -1;


	// First get the size of the devices buffer
	errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Failed call to clGetContextInfo(...,GL_CONTEXT_DEVICES,...)";
		return NULL;
	}

	if (deviceBufferSize <= 0)
	{
		std::cerr << "No devices available.";
		return NULL;
	}


	// Allocate memory for the devices buffer
	devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
	errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
	if (errNum != CL_SUCCESS)
	{
		delete [] devices;
		std::cerr << "Failed to get device IDs";
		return NULL;
	}


	// In this example, we just choose the first available device.  In a
	// real program, you would likely use all available devices or choose
	// the highest performance device based on OpenCL device queries
	commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
	if (commandQueue == NULL)
	{
		delete [] devices;
		std::cerr << "Failed to create commandQueue for device 0";
		return NULL;
	}

	*device = devices[0];
	delete [] devices;
	return commandQueue;
}

///
//  Create an OpenCL program from the kernel source file
//
extern "C" cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName)
{
	cl_int errNum;
	cl_program program;


	std::ifstream kernelFile(fileName, std::ios::in);
	if (!kernelFile.is_open())
	{
		std::cerr << "Failed to open file for reading: " << fileName << std::endl;
		return NULL;
	}


	std::ostringstream oss;
	oss << kernelFile.rdbuf();

	std::string srcStdStr = oss.str();
	const char *srcStr = srcStdStr.c_str();
	program = clCreateProgramWithSource(context, 1,
			(const char**)&srcStr,
			NULL, NULL);


	if (program == NULL)
	{
		std::cerr << "Failed to create CL program from source." << std::endl;
		return NULL;
	}

	errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		// Determine the reason for the error
		char buildLog[16384];
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
				sizeof(buildLog), buildLog, NULL);

		std::cerr << "Error in kernel: " << std::endl;
		std::cerr << buildLog;
		clReleaseProgram(program);
		return NULL;
	}
	return program;
}



///
//  Create memory objects used as the arguments to the kernel
//  The kernel takes three arguments: result (output), a (input),
//  and b (input)
//
extern "C" bool CreateMemObjects(cl_context context, cl_mem memObjects[3],
		float *a, float *b)
{
	memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			sizeof(float) * ARRAY_SIZE, a, NULL);
	memObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			sizeof(float) * ARRAY_SIZE, b, NULL);
	memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE,
			sizeof(int) * ARRAY_SIZE, NULL, NULL);

	if (memObjects[0] == NULL || memObjects[1] == NULL || memObjects[2] == NULL)
	{
		std::cerr << "Error creating memory objects." << std::endl;
		return false;
	}
	return true;
}


///
//  Cleanup any created OpenCL resources
//
extern "C" void Cleanup(cl_context context, cl_command_queue commandQueue,
		cl_program program, cl_kernel kernel, cl_mem memObjects[3])
{
	for (int i = 0; i < 3; i++)
	{
		if (memObjects[i] != 0)
			clReleaseMemObject(memObjects[i]);
	}
	if (commandQueue != 0)
		clReleaseCommandQueue(commandQueue);

	if (kernel != 0)
		clReleaseKernel(kernel);

	if (program != 0)
		clReleaseProgram(program);

	if (context != 0)
		clReleaseContext(context);

}







/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Default Process
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

extern "C" int default_cs(struct qhgpu_service_request *sr)
{
	printf("[libsrv_default] Info: default_cs\n");
	return 0;
}


// launch and copy result to sr->dout
extern "C" int default_launch(struct qhgpu_service_request *sr)
{
	printf("[libsrv_default] Info: default_launch !!!\n");




	///step 1 create context
	// Create an OpenCL context on first available platform
	context = CreateContext();
	if (context == NULL)
	{
		std::cerr << "Failed to create OpenCL context." << std::endl;
		return 1;
	}


	///step 2 create command queue
	// Create a command-queue on the first device available
	// on the created context
	commandQueue = CreateCommandQueue(context, &device);
	if (commandQueue == NULL)
	{
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}


	///step 3 create program
	// Create OpenCL program from HelloWorld.cl kernel source
	program = CreateProgram(context, device, "HelloWorld.cl");
	if (program == NULL)
	{
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}


	///step 4 create kernel
	// Create OpenCL kernel
	kernel = clCreateKernel(program, "hello_kernel", NULL);
	if (kernel == NULL)
	{
		std::cerr << "Failed to create kernel" << std::endl;
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}



	/////////////////// get data from kernel space
	float* test_in;
	if(sr->hin!=NULL){
		test_in = (float*)sr->hin;
		int i=0;

		for(i=0;i<5;i++)
			printf("test_in : %f ~~~~~ \n",test_in[i]);
	}
	else
		printf("sr->hin is null !!! \n");
	/////////////////////////////////////////////////////////



	///step 5 memory management
	// Create OpenCL kernel
	if (!CreateMemObjects(context, memObjects, test_in, test_in))
	{
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	// Set the kernel arguments (result, a, b)
	errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &memObjects[0]);
	errNum |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &memObjects[1]);
	errNum |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &memObjects[2]);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error setting kernel arguments." << std::endl;
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	///step 6 Execution
	size_t globalWorkSize[1] = { ARRAY_SIZE };
	size_t localWorkSize[1] = { 1 };


	// Queue the kernel up for execution across the array
	errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL,
			globalWorkSize, localWorkSize,
			0, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error queuing kernel for execution." << std::endl;
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}


	return 0;
}


extern "C" int default_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_default] Info: default_post\n");


	///step 7 POST
	// Read the output buffer back to the Host
	errNum = clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE,
			0, ARRAY_SIZE * sizeof(int), result,
			0, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error reading result buffer." << std::endl;
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}
	// Output the result buffer
	for (int i = 0; i < ARRAY_SIZE; i++)
	{
		printf("result : %d \n",result[i]);
	}

	std::cout << std::endl;
	std::cout << "Executed program succesfully." << std::endl;
	Cleanup(context, commandQueue, program, kernel, memObjects);

	memcpy(sr->hout, result, (ARRAY_SIZE-5) * ( sizeof( int ) ));



	return 0;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////







static struct qhgpu_service default_srv;

extern "C" int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*),
		cl_context ctx,
		cl_device_id* dv)
{
	printf("[libsrv_default] Info: init test service !!!!\n");
	sprintf(default_srv.name, "default_service");

	default_srv.sid = 1;
	default_srv.compute_size = default_cs;


	default_srv.launch = default_launch;
	default_srv.post = default_post;


	return reg_srv(&default_srv, lh);
}

extern "C" int finit_service(void *lh, int (*unreg_srv)(const char*))
{
	printf("[libsrv_default] Info: finit test service\n");
	return unreg_srv(default_srv.name);
}
