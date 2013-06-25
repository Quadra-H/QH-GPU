/*
cl_helper is Created the foundation for Init function, including
to use the OpenCL API that handles exceptions.
*/

#include <CL/cl.h>
#include <vector>
#include <string>

using std::string;
using std::ifstream;
using std::cerr;
using std::endl;
using std::cout;

#define WORK_DIM 1

struct oclHandlesStruct{
	cl_context context;
	cl_device_id *devices;
	cl_command_queue queue;
	cl_program program;
	cl_int cl_status;
	std::vector<cl_kernel> kernel;
	std::string error_str;
	cl_mem pinned_mem_out;
	cl_mem pinned_mem_in;
};

struct oclHandlesStruct oclHandles;

int number_devices = 0;
int work_group_size = 192;
int device_id_inused = 0;

int total_kernels = 1;
string kernel_names[1] = { "vector_add" };

char kernel_file[100] = "vector_add_kernel.cl";

char device_type[3];
int device_id = 0;

string FileToString(const string fileName){
	 ifstream f(fileName.c_str(), ifstream::in | ifstream::binary);

	    try
	    {
	        size_t size;
	        char*  str;
	        string s;

	        if(f.is_open())
	        {
	            size_t fileSize;
	            f.seekg(0, ifstream::end);
	            size = fileSize = f.tellg();
	            f.seekg(0, ifstream::beg);

	            str = new char[size+1];
	            if (!str) throw(string("Could not allocate memory"));

	            f.read(str, fileSize);
	            f.close();
	            str[size] = '\0';

	            s = str;
	            delete [] str;
	            return s;
	        }
	    }
	    catch(string msg)
	    {
	        cerr << "Exception caught in FileToString(): " << msg << endl;
	        if(f.is_open())
	            f.close();
	    }
	    catch(...)
	    {
	        cerr << "Exception caught in FileToString()" << endl;
	        if(f.is_open())
	            f.close();
	    }
	    string errorMsg = "FileToString()::Error: Unable to open file " + fileName;
	    throw(errorMsg);
}


void _clInit()
{
	int DEVICE_ID_INUSED = device_id_inused;
	cl_int resultCL;

	oclHandles.context = NULL;
	oclHandles.devices = NULL;
	oclHandles.queue = NULL;
	oclHandles.program = NULL;

	cl_uint deviceListSize;

	cl_uint numPlatforms;
	cl_platform_id targetPlatform = NULL;

	//First, find the available platforms.
	resultCL = clGetPlatformIDs(0, NULL, &numPlatforms);
	if(resultCL != CL_SUCCESS){
		throw(string("InitCL()::Error : Getting number of platforms (clGetPlatformIDs)"));
	}

	if(!(numPlatforms >0)){
		throw(string("InitCL()::Error : No platforms found (clGetPlatformIDs"));
	}

	cl_platform_id * allPlatforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));

	resultCL = clGetPlatformIDs(numPlatforms, allPlatforms, NULL);
	if(resultCL != CL_SUCCESS){
		throw(string("InitCL()::Error : Getting platform ids (clGetPlatfromIDs"));
	}

	targetPlatform = allPlatforms[0]; // the default value of target platform to platform first choice.
	for(int i = 0; i < numPlatforms; i++){
		char pbuff[128];
		resultCL = clGetPlatformInfo(allPlatforms[i], CL_PLATFORM_VENDOR, sizeof(pbuff), pbuff, NULL);
		if(resultCL != CL_SUCCESS)
			throw(string("InitCL()::Error : Getting platform info (clGetPlatformInfo"));
		printf("vendor is %s\n", pbuff);
	}
	free(allPlatforms);

	//Second, OpenCL Device Select, but Device type currently set to the GPU. To replace the part. CPU can be in.
	oclHandles.cl_status = clGetDeviceIDs(targetPlatform, CL_DEVICE_TYPE_GPU, 0, NULL, &deviceListSize);
	if(oclHandles.cl_status != CL_SUCCESS){
		throw(string("exception in _clInit -> clGetDeviceIDs"));
	}

	if(deviceListSize == 0){
		throw(string("InitCL()::Error : No devices found."));
	}

	oclHandles.devices = (cl_device_id*)malloc(deviceListSize);

	if(oclHandles.devices == 0){
		throw(string("InitCL()::Error : Could not allocate memory"));
	}

	oclHandles.cl_status = clGetDeviceIDs(targetPlatform, CL_DEVICE_TYPE_GPU, deviceListSize, oclHandles.devices, NULL);

	if(oclHandles.cl_status != CL_SUCCESS){
		throw(string("exception in _clInit -> clGetDeviceIDs -2"));
	}

	//Third, Create Context
	cl_context_properties cprops[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)targetPlatform, 0 };
	oclHandles.context = clCreateContextFromType(cprops,CL_DEVICE_TYPE_GPU, NULL, NULL, &resultCL);

	if((resultCL != CL_SUCCESS) || (oclHandles.context == NULL))
		throw(string("InitCL::Error : Creating Context (clCreateContextFromType"));

	//Fourth, Create OpenCL Command queue
	oclHandles.queue = clCreateCommandQueue(oclHandles.context, oclHandles.devices[DEVICE_ID_INUSED], 0, &resultCL);

	if((resultCL != CL_SUCCESS) || (oclHandles.queue == NULL)){
		throw(string("InitCL::Error : Creating Command Queue (clCreateCommandQueue"));
	}

	string  source_str = FileToString(kernel_file);
	const char * source    = source_str.c_str();
	size_t sourceSize[]    = { source_str.length() };

	//Program object build
	oclHandles.program = clCreateProgramWithSource(oclHandles.context, 1, &source, sourceSize, &resultCL);
	if((resultCL != CL_SUCCESS) || oclHandles.program == NULL){
		throw(string("InitCL()::Error : Loading Binary into cl_program. (clCreateProgramWithSource)"));
	}

	//string options= "-cl-nv-verbose";
	resultCL = clBuildProgram(oclHandles.program, deviceListSize, oclHandles.devices, NULL,NULL,NULL);
	if((resultCL != CL_SUCCESS) || (oclHandles.program == NULL)){
		cerr << "InitCL()::Error : In clBuildProgram" << endl;
		size_t length;

		resultCL = clGetProgramBuildInfo(oclHandles.program, oclHandles.devices[DEVICE_ID_INUSED],CL_PROGRAM_BUILD_LOG, 0, NULL, &length);
		if(resultCL != CL_SUCCESS){
			throw(string("InitCL()::Error : Getting program build info (clGetProgramBuildInfo)"));
		}

		char * buffer = (char*)malloc(length);
		resultCL = clGetProgramBuildInfo(oclHandles.program, oclHandles.devices[DEVICE_ID_INUSED],CL_PROGRAM_BUILD_LOG,length, buffer, NULL);

		if(resultCL != CL_SUCCESS){
			throw(string("InitCL::Error : Getting Program build info (clGetProgramBuildInfo)"));

		}
		cerr << buffer << endl;
		free(buffer);
		throw(string("InitCL()::Error : Build Program (clBuildProgram)"));
	}

	for(int nKernel = 0; nKernel < total_kernels; nKernel++){
		cl_kernel kernel = clCreateKernel(oclHandles.program, (kernel_names[nKernel]).c_str(), &resultCL);

		if((resultCL != CL_SUCCESS) || (kernel == NULL)){
			string errorMsg = "InitCL()::Error : Creating Kernel(clCreateKernel) \"" + kernel_names[nKernel] + "\"";
			throw(errorMsg);
		}
		oclHandles.kernel.push_back(kernel);
	}
}

void _clSetArgs(int kernel_id, int arg_idx, void * d_mem, int size =0) throw(string){
	if(!size)
	{
		oclHandles.cl_status = clSetKernelArg(oclHandles.kernel[kernel_id], arg_idx, sizeof(d_mem), &d_mem);

#ifdef ERRMSG
	oclHandles.error_str = "exception in _clSetKernelArg()";
	switch(oclHandles.cl_status)
	{
		case CL_INVALID_KERNEL:
			oclHandles.error_str += "CL_INVALID_KERNEL";
			break;
		case CL_INVALID_ARG_INDEX:
			oclHandles.error_str += "CL_INVALID_ARG_INDEX";
			break;
		case CL_INVALID_ARG_VALUE:
			oclHandles.error_str += "CL_INVALID_ARG_VALUE";
			break;
		case CL_INVALID_MEM_OBJECT:
			oclHandles.error_str += "CL_INVALID_MEM_OBJECT";
			break;
		case CL_INVALID_SAMPLER:
			oclHandles.error_str += "CL_INVALID_SAMPLER";
			break;
		case CL_INVALID_ARG_SIZE:
			oclHandles.error_str += "CL_INVALID_ARG_SIZE";
			break;
		case CL_OUT_OF_RESOURCES:
			oclHandles.error_str += "CL_OUT_OF_RESOURCES";
			break;
		case CL_OUT_OF_HOST_MEMORY:
			oclHandles.error_str += "CL_OUT_OF_HOST_MEMORY";
			break;
		default:
			oclHandles.error_str += "Unknown reason";
			break;
		}
	}
	if(oclHandles.cl_status != CL_SUCCESS){
		throw(oclHandles.error_str);
	}
	#endif
	}
	else{
		oclHandles.cl_status = clSetKernelArg(oclHandles.kernel[kernel_id], arg_idx, size, d_mem);
		#ifdef ERRMSG
		oclHandles.error_str = "exception in _clSetKernelArg() ";
		switch(oclHandles.cl_status){
			case CL_INVALID_KERNEL:
				oclHandles.error_str += "CL_INVALID_KERNEL";
				break;
			case CL_INVALID_ARG_INDEX:
				oclHandles.error_str += "CL_INVALID_ARG_INDEX";
				break;
			case CL_INVALID_ARG_VALUE:
				oclHandles.error_str += "CL_INVALID_ARG_VALUE";
				break;
			case CL_INVALID_MEM_OBJECT:
				oclHandles.error_str += "CL_INVALID_MEM_OBJECT";
				break;
			case CL_INVALID_SAMPLER:
				oclHandles.error_str += "CL_INVALID_SAMPLER";
				break;
			case CL_INVALID_ARG_SIZE:
				oclHandles.error_str += "CL_INVALID_ARG_SIZE";
				break;
			case CL_OUT_OF_RESOURCES:
				oclHandles.error_str += "CL_OUT_OF_RESOURCES";
				break;
			case CL_OUT_OF_HOST_MEMORY:
				oclHandles.error_str += "CL_OUT_OF_HOST_MEMORY";
				break;
			default:
				oclHandles.error_str += "Unknown reason";
				break;
		}
		if(oclHandles.cl_status != CL_SUCCESS)
			throw(oclHandles.error_str);
		#endif
	}
}

cl_mem _clMalloc(int size, void * h_mem_ptr) throw(string){
	cl_mem d_mem;
	d_mem = clCreateBuffer(oclHandles.context, CL_MEM_READ_ONLY, size, h_mem_ptr, &oclHandles.cl_status);

	if(oclHandles.cl_status != CL_SUCCESS){
		throw(string("exception in _clMalloc"));
	}
	return d_mem;
}

// memory copy Host to Device
void _clMemcpyH2D(cl_mem d_mem, int size, const void *h_mem_ptr) throw(string){
	oclHandles.cl_status = clEnqueueWriteBuffer(oclHandles.queue, d_mem, CL_TRUE, 0, size, h_mem_ptr, 0, NULL, NULL);
	#ifdef ERRMSG
	if(oclHandles.cl_status != CL_SUCCESS)
		throw(string("exception in _clMemcpyH2D"));
	#endif
}


void _clInvokeKernel(int kernel_id, int work_items, int work_group_size) throw(string){
	cl_uint work_dim = WORK_DIM;
	cl_event e[1];
	if(work_items%work_group_size != 0)
		  work_items = work_items + (work_group_size-(work_items%work_group_size));
	size_t local_work_size[] = {work_group_size, 1};
	size_t global_work_size[] = {work_items, 1};
	oclHandles.cl_status = clEnqueueNDRangeKernel(oclHandles.queue, oclHandles.kernel[kernel_id], work_dim, 0, global_work_size, local_work_size, 0 , 0, &(e[0]) );
	#ifdef ERRMSG
	oclHandles.error_str = "exception in _clInvokeKernel() -> ";
	switch(oclHandles.cl_status)
	{
		case CL_INVALID_PROGRAM_EXECUTABLE:
			oclHandles.error_str += "CL_INVALID_PROGRAM_EXECUTABLE";
			break;
		case CL_INVALID_COMMAND_QUEUE:
			oclHandles.error_str += "CL_INVALID_COMMAND_QUEUE";
			break;
		case CL_INVALID_KERNEL:
			oclHandles.error_str += "CL_INVALID_KERNEL";
			break;
		case CL_INVALID_CONTEXT:
			oclHandles.error_str += "CL_INVALID_CONTEXT";
			break;
		case CL_INVALID_KERNEL_ARGS:
			oclHandles.error_str += "CL_INVALID_KERNEL_ARGS";
			break;
		case CL_INVALID_WORK_DIMENSION:
			oclHandles.error_str += "CL_INVALID_WORK_DIMENSION";
			break;
		case CL_INVALID_GLOBAL_WORK_SIZE:
			oclHandles.error_str += "CL_INVALID_GLOBAL_WORK_SIZE";
			break;
		case CL_INVALID_WORK_GROUP_SIZE:
			oclHandles.error_str += "CL_INVALID_WORK_GROUP_SIZE";
			break;
		case CL_INVALID_WORK_ITEM_SIZE:
			oclHandles.error_str += "CL_INVALID_WORK_ITEM_SIZE";
			break;
		case CL_INVALID_GLOBAL_OFFSET:
			oclHandles.error_str += "CL_INVALID_GLOBAL_OFFSET";
			break;
		case CL_OUT_OF_RESOURCES:
			oclHandles.error_str += "CL_OUT_OF_RESOURCES";
			break;
		case CL_MEM_OBJECT_ALLOCATION_FAILURE:
			oclHandles.error_str += "CL_MEM_OBJECT_ALLOCATION_FAILURE";
			break;
		case CL_INVALID_EVENT_WAIT_LIST:
			oclHandles.error_str += "CL_INVALID_EVENT_WAIT_LIST";
			break;
		case CL_OUT_OF_HOST_MEMORY:
			oclHandles.error_str += "CL_OUT_OF_HOST_MEMORY";
			break;
		default:
			oclHandles.error_str += "Unknown reason";
			break;
	}
	if(oclHandles.cl_status != CL_SUCCESS)
		throw(oclHandles.error_str);
	#endif
}

// memory copy Device to Host
void _clMemcpyD2H(cl_mem d_mem, int size, void * h_mem) throw(string){
	oclHandles.cl_status = clEnqueueReadBuffer(oclHandles.queue, d_mem, CL_TRUE, 0, size, h_mem, 0,NULL,NULL);
	#ifdef ERRMSG
		oclHandles.error_str = "exception in _clCpyMemD2H -> ";
		switch(oclHandles.cl_status){
			case CL_INVALID_COMMAND_QUEUE:
				oclHandles.error_str += "CL_INVALID_COMMAND_QUEUE";
				break;
			case CL_INVALID_CONTEXT:
				oclHandles.error_str += "CL_INVALID_CONTEXT";
				break;
			case CL_INVALID_MEM_OBJECT:
				oclHandles.error_str += "CL_INVALID_MEM_OBJECT";
				break;
			case CL_INVALID_VALUE:
				oclHandles.error_str += "CL_INVALID_VALUE";
				break;
			case CL_INVALID_EVENT_WAIT_LIST:
				oclHandles.error_str += "CL_INVALID_EVENT_WAIT_LIST";
				break;
			case CL_MEM_OBJECT_ALLOCATION_FAILURE:
				oclHandles.error_str += "CL_MEM_OBJECT_ALLOCATION_FAILURE";
				break;
			case CL_OUT_OF_HOST_MEMORY:
				oclHandles.error_str += "CL_OUT_OF_HOST_MEMORY";
				break;
			default:
				oclHandles.error_str += "Unknown reason";
				break;
		}
		if(oclHandles.cl_status != CL_SUCCESS)
			throw(oclHandles.error_str);
	#endif
}

void _clFinish() throw(string){
	oclHandles.cl_status = clFinish(oclHandles.queue);
	#ifdef ERRMSG
	oclHandles.error_str = "exception in _clFinish";
	switch(oclHandles.cl_status){
		case CL_INVALID_COMMAND_QUEUE:
			oclHandles.error_str += "CL_INVALID_COMMAND_QUEUE";
			break;
		case CL_OUT_OF_RESOURCES:
			oclHandles.error_str += "CL_OUT_OF_RESOURCES";
			break;
		case CL_OUT_OF_HOST_MEMORY:
			oclHandles.error_str += "CL_OUT_OF_HOST_MEMORY";
			break;
		default:
			oclHandles.error_str += "Unknown reasons";
			break;

	}
	if(oclHandles.cl_status!=CL_SUCCESS){
		throw(oclHandles.error_str);
	}
	#endif
}

void _clFree(cl_mem ob) throw(string){
	if(ob!=NULL)
		oclHandles.cl_status = clReleaseMemObject(ob);
	#ifdef ERRMSG
	oclHandles.error_str = "exception in _clFree() ->";
	switch(oclHandles.cl_status)
	{
		case CL_INVALID_MEM_OBJECT:
			oclHandles.error_str += "CL_INVALID_MEM_OBJECT";
			break;
		case CL_OUT_OF_RESOURCES:
			oclHandles.error_str += "CL_OUT_OF_RESOURCES";
			break;
		case CL_OUT_OF_HOST_MEMORY:
			oclHandles.error_str += "CL_OUT_OF_HOST_MEMORY";
			break;
		default:
			oclHandles.error_str += "Unknown reason";
			break;
	}
    if (oclHandles.cl_status!= CL_SUCCESS)
       throw(oclHandles.error_str);
	#endif
}

void _clRelease()
{
    bool errorFlag = false;

    for (int nKernel = 0; nKernel < oclHandles.kernel.size(); nKernel++)
    {
        if (oclHandles.kernel[nKernel] != NULL)
        {
            cl_int resultCL = clReleaseKernel(oclHandles.kernel[nKernel]);
            if (resultCL != CL_SUCCESS)
            {
                cerr << "ReleaseCL()::Error: In clReleaseKernel" << endl;
                errorFlag = true;
            }
            oclHandles.kernel[nKernel] = NULL;
        }
        oclHandles.kernel.clear();
    }

    if (oclHandles.program != NULL)
    {
        cl_int resultCL = clReleaseProgram(oclHandles.program);
        if (resultCL != CL_SUCCESS)
        {
            cerr << "ReleaseCL()::Error: In clReleaseProgram" << endl;
            errorFlag = true;
        }
        oclHandles.program = NULL;
    }

    if (oclHandles.queue != NULL)
    {
        cl_int resultCL = clReleaseCommandQueue(oclHandles.queue);
        if (resultCL != CL_SUCCESS)
        {
            cerr << "ReleaseCL()::Error: In clReleaseCommandQueue" << endl;
            errorFlag = true;
        }
        oclHandles.queue = NULL;
    }

    free(oclHandles.devices);

    if (oclHandles.context != NULL)
    {
        cl_int resultCL = clReleaseContext(oclHandles.context);
        if (resultCL != CL_SUCCESS)
        {
            cerr << "ReleaseCL()::Error: In clReleaseContext" << endl;
            errorFlag = true;
        }
        oclHandles.context = NULL;
    }

    if (errorFlag) throw(string("ReleaseCL()::Error encountered."));
}



