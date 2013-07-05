#include <CL/cl.h>
#include <stdio.h>



cl_context Context;
cl_command_queue CommandQueue;
cl_device_id *devices;

const int QHGPU_MEM_SIZE = 1024;

void* gpu_alloc_pinned_mem(){

	printf("gpu_alloc_pinned_mem !!!!!\n");
	cl_mem cmPinnedData = NULL;
	int* h_data = NULL;

	cl_mem cmDevData = NULL;
	cl_int clErrNum;


	// Create a host buffer
	cmPinnedData = clCreateBuffer(Context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, QHGPU_MEM_SIZE, NULL, &clErrNum);
	if(clErrNum != CL_SUCCESS){
		printf("exception in gpu_init -> clCreateBuffer of host\n");
		//throw(std::string("exception in gpu_init -> clCreateBuffer of host"));
		return NULL;
	}


	// Get a mapped pointer
	h_data = (int*)clEnqueueMapBuffer(CommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, QHGPU_MEM_SIZE, 0, NULL, NULL, &clErrNum);
	if(clErrNum != CL_SUCCESS){
		printf("exception in gpu_init -> clEnqueueMapBuffer\n");
		//throw(std::string("exception in gpu_init -> clEnqueueMapBuffer"));
		return NULL;
	}


	//initialize
	int i = 0;
	for(i = 0; i < QHGPU_MEM_SIZE; i++)
	{
		h_data[i] = (int)(i & 0xff);
	}
	printf("%p\n", h_data);

	return h_data;
}


int gpu_init()
{


	int device_id_inused = 0;

	cl_uint num_platforms;
	cl_uint clDeviceCount;


	cl_platform_id *clPlatformIDs;
	cl_int clErrNum;
	cl_platform_id clSelectedPlatformID = NULL;

	clErrNum = clGetPlatformIDs(0, NULL, &num_platforms);
	printf("test1");
	if(clErrNum != CL_SUCCESS){
		//throw(std::string("exception in main -> clGetPlatformIDs"));
		return -1;
	}
	else
	{
		printf("test2");
		if(num_platforms < 0){
			//throw(std::string("No OpenCL platform found!"));
			return -1;
		}

		clPlatformIDs = (cl_platform_id*)malloc(num_platforms * sizeof(cl_platform_id));
		clErrNum = clGetPlatformIDs(num_platforms, clPlatformIDs, NULL);
		if(clErrNum != CL_SUCCESS){
			//throw(std::string("No Getting platform ids!"));
			return -1;
		}


		// the default value of target platform to platform first choice.
		clSelectedPlatformID = clPlatformIDs[0];
		char chBuffer[1024];
		cl_uint ui = 0;
		for(ui = 0; ui < num_platforms; ui++){
			clErrNum = clGetPlatformInfo(clPlatformIDs[ui], CL_PLATFORM_NAME, sizeof(chBuffer), chBuffer, NULL);
			if(clErrNum != CL_SUCCESS){
				//throw(std::string("No corresponding platform!"));
				return -1;
			}
		}
		free(clPlatformIDs);
	}

	printf("test3");
	clErrNum = clGetDeviceIDs(clSelectedPlatformID, CL_DEVICE_TYPE_GPU, 0, NULL, &clDeviceCount);
	if(clErrNum != CL_SUCCESS){

		printf("exception in main -> clGetDeviceIDs");
		//throw(std::string("exception in main -> clGetDeviceIDs"));
		return -1;
	}
	else if(clDeviceCount == 0){
		printf("There are no devices supporting OpenCL (return code %i)\n", clErrNum);
		//throw(std::string("There are no devices supporting OpenCL (return code %i)\n"), clErrNum);
		return -1;
	}
	devices = (cl_device_id*) malloc(sizeof(cl_device_id) * clDeviceCount);
	clErrNum = clGetDeviceIDs (clSelectedPlatformID, CL_DEVICE_TYPE_GPU, clDeviceCount, devices, &clDeviceCount);

	Context = clCreateContext(0, clDeviceCount, devices, NULL, NULL, NULL);
	if(Context == (cl_context)0){
		printf("Failed to create OpenCL context!\n");
		//throw(std::string("Failed to create OpenCL context!\n"));
		return -1;
	}

	CommandQueue = clCreateCommandQueue(Context, devices[device_id_inused], CL_QUEUE_PROFILING_ENABLE, NULL);
	if((clErrNum != CL_SUCCESS) || (CommandQueue == NULL)){
		printf("Failed to create command queue!\n");
		//throw(std::string("Failed to create command queue!\n"));
		return -1;
	}





	cl_mem cmPinnedData = NULL;
	int* h_data = NULL;
	clEnqueueMapBuffer(CommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, QHGPU_MEM_SIZE, 0, NULL, NULL, &clErrNum);
	if(clErrNum != CL_SUCCESS){
		//throw(std::string("exception in gpu_init -> clEnqueueMapBuffer"));
		return -1;
	}


	//cl_int clErrNum;



	// Create a host buffer
	cmPinnedData = clCreateBuffer(Context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, QHGPU_MEM_SIZE, NULL, &clErrNum);
	if(clErrNum != CL_SUCCESS){
		//throw(std::string("exception in gpu_init -> clCreateBuffer of host"));
		return -1;
	}

	/*// Get a mapped pointer
	h_data = (int*)gpu_alloc_pinned_mem();



	cl_mem cmDevData = NULL;
	//initialize
	for(int i = 0; i < QHGPU_MEM_SIZE; i++)
	{
		h_data[i] = (int)(i & 0xff);
	}
	printf("%p\n", h_data);

	// allocate device memory
	cmDevData = clCreateBuffer(Context, CL_MEM_READ_WRITE, QHGPU_MEM_SIZE, NULL, &clErrNum);
	if(clErrNum != CL_SUCCESS){
		throw(std::string("exception in gpu_init -> clCreateBuffer of device"));
		return -1;
	}

	clErrNum = clEnqueueWriteBuffer(CommandQueue, cmDevData, CL_FALSE, 0, QHGPU_MEM_SIZE, h_data, 0, NULL, NULL);
	if(clErrNum != CL_SUCCESS){i
		throw(std::string("exception in gpu_init -> clCreateBuffer of device"));
		return -1;
	}
	clErrNum = clFinish(CommandQueue);
	if(clErrNum != CL_SUCCESS){
		throw(std::string("exception in gpu_init -> clFinish"));
		return -1;
	}*/
}

/*
int main(int argc, char ** argv)
{
	gpu_init();
}*/
