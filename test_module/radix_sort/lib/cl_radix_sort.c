#include<stdio.h>
#include <string.h>
#include "cl_radix_sort.h"
//#include "../../../qhgpu/connector.h"

#define max(a,b) (((a)>(b))?(a):(b))


char* oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t* szFinalLength)
{
	// locals
	FILE* pFileStream = NULL;
	size_t szSourceLength;

	// open the OpenCL source code file
#ifdef _WIN32   // Windows version
	if(fopen_s(&pFileStream, cFilename, "rb") != 0)
	{
		return NULL;
	}
#else           // Linux version
	pFileStream = fopen(cFilename, "rb");
	if(pFileStream == 0)
	{
		printf("cannot find file: %s !!!! \n",cFilename);
		return NULL;
	}
#endif

	size_t szPreambleLength = strlen(cPreamble);

	// get the length of the source code
	fseek(pFileStream, 0, SEEK_END);
	szSourceLength = ftell(pFileStream);
	fseek(pFileStream, 0, SEEK_SET);

	// allocate a buffer for the source code string and read it in

	char* cSourceString = (char *)malloc(szSourceLength + szPreambleLength + 1);
	memcpy(cSourceString, cPreamble, szPreambleLength);
	if (fread((cSourceString) + szPreambleLength, szSourceLength, 1, pFileStream) != 1)
	{
		fclose(pFileStream);
		free(cSourceString);
		return 0;
	}

	// close the file and return the total length of the combined (preamble + source) string
	fclose(pFileStream);
	if(szFinalLength != 0)
	{
		*szFinalLength = szSourceLength + szPreambleLength;
	}

	cSourceString[szSourceLength + szPreambleLength] = '\0';

	return cSourceString;
}


void build_cl_radix_sort(cl_context ctx,
		cl_device_id* devices){

	context = ctx;
	num_device= devices[0];

	cl_int status;

	command_que = clCreateCommandQueue(
				context,
				num_device,
				CL_QUEUE_PROFILING_ENABLE,
				&status);
		assert (status == CL_SUCCESS);


	cl_int err;
	size_t szKernelLength;
	char* prog = NULL;
	const char* cSourceFile = "./cl_radix_sort.cl";//커널파일 이름
	printf("oclLoadProgSource (%s)...\n", cSourceFile);
	prog = oclLoadProgSource(cSourceFile, "", &szKernelLength);



	program = clCreateProgramWithSource(context, 1, (const char **)&prog, NULL, &err);
	if (!program) {
		printf("Error: Failed to create compute program!\n");
	}
	assert(err == CL_SUCCESS);


	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		size_t len;
		char buffer[2048];
		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, num_device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		assert( err == CL_SUCCESS);
	}


	ckHistogram = clCreateKernel(program, "histogram", &err);
	assert(err == CL_SUCCESS);
	ckScanHistogram = clCreateKernel(program, "scanhistograms", &err);
	assert(err == CL_SUCCESS);
	ckPasteHistogram = clCreateKernel(program, "pastehistograms", &err);
	assert(err == CL_SUCCESS);
	ckReorder = clCreateKernel(program, "reorder", &err);
	assert(err == CL_SUCCESS);
	ckTranspose = clCreateKernel(program, "transpose", &err);
	assert(err == CL_SUCCESS);

	printf("Create Kernel finished !!\n");

}



void init_cl_radix_sort(
		int nkeys){



	cl_int err;


	cl_int status;

	/**/



	nkeys_rounded=nkeys;
	// check some conditions
	assert(_TOTALBITS % _BITS == 0);
	assert(nkeys % (_GROUPS * _ITEMS) == 0);
	assert( (_GROUPS * _ITEMS * _RADIX) % _HISTOSPLIT == 0);
	assert(pow(2,(int) log2(_GROUPS)) == _GROUPS);
	assert(pow(2,(int) log2(_ITEMS)) == _ITEMS);

	// init the timers
	histo_time=0;
	scan_time=0;
	reorder_time=0;
	transpose_time=0;










	//printf("Construct the random list\n");
	// construction of a random list
	uint maxint=_MAXINT;
	assert(_MAXINT != 0);


	h_checkKeys = (uint*)malloc(sizeof(uint)*nkeys);
	h_Permut = (uint*)malloc(sizeof(uint)*nkeys);
	// construction of the initial permutation
	for(uint i = 0; i < nkeys; i++){

		//printf("%d, ",i);
		h_Permut[i] = i;
		h_checkKeys[i]=h_keys[i];

	}


	printf("Send to the GPU\n");
	// copy on the GPU
	d_inKeys  = clCreateBuffer(context,
			CL_MEM_READ_WRITE,
			sizeof(uint)* nkeys ,
			NULL,
			&err);
	assert(err == CL_SUCCESS);

	d_outKeys  = clCreateBuffer(context,
			CL_MEM_READ_WRITE,
			sizeof(uint)* nkeys ,
			NULL,
			&err);
	assert(err == CL_SUCCESS);

	d_inPermut  = clCreateBuffer(context,
			CL_MEM_READ_WRITE,
			sizeof(uint)* nkeys ,
			NULL,
			&err);
	assert(err == CL_SUCCESS);

	d_outPermut  = clCreateBuffer(context,
			CL_MEM_READ_WRITE,
			sizeof(uint)* nkeys ,
			NULL,
			&err);
	assert(err == CL_SUCCESS);



	////////////////////////////////////////////////////////////////////////////////
	//copy the two previous vectors to the device
	//cl_radix_host2gpu();
	////////////////////////////////////////////////////////////////////////////////
	status = clEnqueueWriteBuffer( command_que,
			d_inKeys,
			CL_TRUE, 0,
			sizeof(uint)  * nkeys,
			h_keys,
			0, NULL, NULL );



	if(status == CL_INVALID_COMMAND_QUEUE){
		printf("if command_queue is not a valid command-queue.1 \n");

	}else if(status == CL_INVALID_CONTEXT){
		printf("if command_queue is not a valid command-queue.2 \n");
	}else if(status == CL_INVALID_MEM_OBJECT){
		printf("if command_queue is not a valid command-queue.3 \n");
	}else if(status == CL_INVALID_VALUE){
		printf("if command_queue is not a valid command-queue.4 \n");
	}else if(status == CL_INVALID_EVENT_WAIT_LIST){
		printf("if command_queue is not a valid command-queue.5 \n");
	}else if(status == CL_MISALIGNED_SUB_BUFFER_OFFSET){
		printf("if command_queue is not a valid command-queue. 6\n");
	}else if(status == CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST){
		printf("if command_queue is not a valid command-queue.7 \n");
	}else if(status == CL_MEM_OBJECT_ALLOCATION_FAILURE){
		printf("if command_queue is not a valid command-queue.8 \n");
	}else if(status == CL_OUT_OF_RESOURCES){
		printf("if command_queue is not a valid command-queue. 9\n");
	}else if(status == CL_OUT_OF_HOST_MEMORY){
		printf("if command_queue is not a valid command-queue.10 \n");
	}

	assert (status == CL_SUCCESS);
	clFinish(command_que);  // wait end of read

	status = clEnqueueWriteBuffer( command_que,
			d_inPermut,
			CL_TRUE, 0,
			sizeof(uint)  * nkeys,
			h_Permut,
			0, NULL, NULL );

	assert (status == CL_SUCCESS);
	clFinish(command_que);  // wait end of read
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////







	// allocate the histogram on the GPU
	d_Histograms  = clCreateBuffer(context,
			CL_MEM_READ_WRITE,
			sizeof(uint)* _RADIX * _GROUPS * _ITEMS,
			NULL,
			&err);
	assert(err == CL_SUCCESS);


	// allocate the auxiliary histogram on GPU
	d_globsum  = clCreateBuffer(context,
			CL_MEM_READ_WRITE,
			sizeof(uint)* _HISTOSPLIT,
			NULL,
			&err);
	assert(err == CL_SUCCESS);

	// temporary vector when the sum is not needed
	d_temp  = clCreateBuffer(context,
			CL_MEM_READ_WRITE,
			sizeof(uint)* _HISTOSPLIT,
			NULL,
			&err);
	assert(err == CL_SUCCESS);

	cl_radix_resize(nkeys);


	// we set here the fixed arguments of the OpenCL kernels
	// the changing arguments are modified elsewhere in the class

	//void histogram(const __global int* d_Keys,__global int* d_Histograms,
	//	const int pass,	__local int* loc_histo,	const int n)
	err = clSetKernelArg(ckHistogram, 1, sizeof(cl_mem), &d_Histograms);
	assert(err == CL_SUCCESS);
	err = clSetKernelArg(ckHistogram, 3, sizeof(uint)*_RADIX*_ITEMS, NULL);
	assert(err == CL_SUCCESS);



	// err = clSetKernelArg(ckHistogram, 3, sizeof(uint)*_ITEMS, NULL);
	// assert(err == CL_SUCCESS);

	err = clSetKernelArg(ckPasteHistogram, 0, sizeof(cl_mem), &d_Histograms);
	assert(err == CL_SUCCESS);

	err = clSetKernelArg(ckPasteHistogram, 1, sizeof(cl_mem), &d_globsum);
	assert(err == CL_SUCCESS);

	err = clSetKernelArg(ckReorder, 2, sizeof(cl_mem), &d_Histograms);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckReorder, 6,
			sizeof(uint)* _RADIX * _ITEMS ,
			NULL); // mem cache
	assert(err == CL_SUCCESS);


}












// resize the sorted vector
void cl_radix_resize(int nn){


	if (VERBOSE){
		printf("Resize to %d \n",nn);
	}
	printf("cl_radix_resize : %d %d \n",nkeys, nn);
	nkeys=nn;
	assert(nn <= nkeys);

	if (VERBOSE){
		printf("Resize to %d \n",nn);
	}
	nkeys=nn;

	// length of the vector has to be divisible by (_GROUPS * _ITEMS)
	int reste=nkeys % (_GROUPS * _ITEMS);
	nkeys_rounded=nkeys;
	cl_int err;
	unsigned int pad[_GROUPS * _ITEMS];
	for(int ii=0;ii<_GROUPS * _ITEMS;ii++){
		pad[ii]=_MAXINT-(unsigned int)1;
	}
	if (reste !=0) {
		nkeys_rounded=nkeys-reste+(_GROUPS * _ITEMS);
		// pad the vector with big values
		assert(nkeys_rounded <= nkeys);
		err = clEnqueueWriteBuffer(command_que,
				d_inKeys,
				CL_TRUE, sizeof(uint)*nkeys,
				sizeof(uint) *(_GROUPS * _ITEMS - reste) ,
				pad,
				0, NULL, NULL);
		//cout << nkeys<<" "<<nkeys_rounded<<endl;
		assert(err == CL_SUCCESS);
	}

}


// transpose the list for faster memory access
void cl_radix_transpose(int nbrow,int nbcol){

	#define _TRANSBLOCK 32 // size of the matrix block loaded into local memeory


	int tilesize=_TRANSBLOCK;

	// if the matrix is too small, avoid using local memory
	if (nbrow%tilesize != 0) tilesize=1;
	if (nbcol%tilesize != 0) tilesize=1;

	if (tilesize == 1) {
		printf("Warning, small list, avoiding cache...\n");
	}

	cl_int err;

	err  = clSetKernelArg(ckTranspose, 0, sizeof(cl_mem), &d_inKeys);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckTranspose, 1, sizeof(cl_mem), &d_outKeys);
	assert(err == CL_SUCCESS);

	err = clSetKernelArg(ckTranspose, 2, sizeof(uint), &nbcol);
	assert(err == CL_SUCCESS);

	err = clSetKernelArg(ckTranspose, 3, sizeof(uint), &nbrow);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckTranspose, 4, sizeof(cl_mem), &d_inPermut);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckTranspose, 5, sizeof(cl_mem), &d_outPermut);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckTranspose, 6, sizeof(uint)*tilesize*tilesize, NULL);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckTranspose, 7, sizeof(uint)*tilesize*tilesize, NULL);
	assert(err == CL_SUCCESS);

	err = clSetKernelArg(ckTranspose, 8, sizeof(uint), &tilesize);
	assert(err == CL_SUCCESS);

	cl_event eve;

	size_t global_work_size[2];
	size_t local_work_size[2];

	assert(nbrow%tilesize == 0);
	assert(nbcol%tilesize == 0);

	global_work_size[0]=nbrow/tilesize;
	global_work_size[1]=nbcol;

	local_work_size[0]=1;
	local_work_size[1]=tilesize;


	err = clEnqueueNDRangeKernel(command_que,
			ckTranspose,
			2,   // two dimensions: rows and columns
			NULL,
			global_work_size,
			local_work_size,
			0, NULL, &eve);

	//exchange the pointers

	// swap the old and new vectors of keys
	cl_mem d_temp;
	d_temp=d_inKeys;
	d_inKeys=d_outKeys;
	d_outKeys=d_temp;

	// swap the old and new permutations
	d_temp=d_inPermut;
	d_inPermut=d_outPermut;
	d_outPermut=d_temp;


	// timing
	clFinish(command_que);

	cl_ulong debut,fin;

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_QUEUED,
			sizeof(cl_ulong),
			(void*) &debut,
			NULL);
	//cout << err<<" , "<<CL_PROFILING_INFO_NOT_AVAILABLE<<endl;
	assert(err== CL_SUCCESS);

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_END,
			sizeof(cl_ulong),
			(void*) &fin,
			NULL);
	assert(err== CL_SUCCESS);

	transpose_time += (float) (fin-debut)/1e9;
}

// global sorting algorithm
void cl_radix_sort(){


	assert(nkeys_rounded <= nkeys);
	assert(nkeys <= nkeys_rounded);
	int nbcol=nkeys_rounded/(_GROUPS * _ITEMS);
	int nbrow= _GROUPS * _ITEMS;


	printf("Start storting %d keys\n",nkeys);
	printf("nbcol: %d, nbrow: %d\n",nbcol,nbrow);

//	if (VERBOSE){
//		printf("Start storting1 %d keys\n",nkeys);
//	}

	if (TRANSPOSE){
		if (VERBOSE) {
			printf("Transpose\n");
		}
		cl_radix_transpose(nbrow,nbcol);
	}


	for(uint pass=0;pass<_PASS;pass++){
		if (VERBOSE) {
			printf("pass %d\n",pass);
		}
		//for(uint pass=0;pass<1;pass++){
		if (VERBOSE) {
			printf("Build histograms \n");
		}
		cl_radix_histogram(pass);
		if (VERBOSE) {
			printf("Scan cl_radix_transposehistograms \n");
		}
		cl_radix_scan_histogram();
		if (VERBOSE) {
			printf("Reorder \n");
		}
		cl_radix_reorder(pass);
	}

	if (TRANSPOSE){
		if (VERBOSE) {
			printf("Transpose back\n");
		}
		cl_radix_transpose(nbcol,nbrow);
	}

	sort_time=histo_time+scan_time+reorder_time+transpose_time;
	if (VERBOSE){
		printf("End sorting\n");
	}
}


// check the computation at the end
void cl_radix_check(){

	printf("Get the data from the GPU\n");

	cl_radix_recup_gpu();

	printf("Test order\n");

	// first see if the final list is ordered
	for(uint i=0;i<nkeys-1;i++){
		if (!(h_keys[i] <= h_keys[i+1])) {
			printf("erreur tri %d %d , %d %d \n", i,h_keys[i],i+1,h_keys[i+1]);
		}
		assert(h_keys[i] <= h_keys[i+1]);
	}

	if (PERMUT) {
		printf("Check the permutation\n");
		// check if the permutation corresponds to the original list
		for(uint i=0;i<nkeys;i++){
			if (!(h_keys[i] == h_checkKeys[h_Permut[i]])) {
				printf("erreur permut %d %d , %d %d \n", i,h_keys[i],i+1,h_keys[i+1]);

			}
		}
	}
	printf("test OK !\n");

}


// get the data from the GPU
void cl_radix_recup_gpu(void){

	cl_int status;

	clFinish(command_que);  // wait end of read

//	printf("\ncl_radix_recup_gpu step 1. %d \n", nkeys);
//
//	printf("\n\n");
//	int i=0;
//	for(i=nkeys-60;i<nkeys-10;i++){
//		printf("%u ,", h_keys[i]);
//	}

	status = clEnqueueReadBuffer( command_que,
			d_inKeys,
			CL_TRUE, 0,
			sizeof(uint)  * nkeys,
			h_keys,
			0, NULL, NULL );

	assert (status == CL_SUCCESS);
	clFinish(command_que);  // wait end of read


//	printf("\n\n after recup \n");
//
//	for(i=nkeys-60;i<nkeys-10;i++){
//		printf("%u ,", h_keys[i]);
//	}
//	printf("\n\n");



	/*printf("cl_radix_recup_gpu step 2. \n");
	status = clEnqueueReadBuffer( command_que,
			d_inPermut,
			CL_TRUE, 0,
			sizeof(uint)  * nkeys,
			h_Permut,
			0, NULL, NULL );

	assert (status == CL_SUCCESS);
	clFinish(command_que);  // wait end of read

	printf("cl_radix_recup_gpu step 3. \n");
	status = clEnqueueReadBuffer( command_que,
			d_Histograms,
			CL_TRUE, 0,
			sizeof(uint)  * _RADIX * _GROUPS * _ITEMS,
			h_Histograms,
			0, NULL, NULL );
	assert (status == CL_SUCCESS);


	printf("cl_radix_recup_gpu step 4. \n");
	status = clEnqueueReadBuffer( command_que,
			d_globsum,
			CL_TRUE, 0,
			sizeof(uint)  * _HISTOSPLIT,
			h_globsum,
			0, NULL, NULL );
	assert (status == CL_SUCCESS);

	printf("cl_radix_recup_gpu step 5. \n");*/

	//clFinish(command_que);  // wait end of read
}

// put the data to the GPU
void cl_radix_host2gpu(){

	cl_int status;

	status = clEnqueueWriteBuffer( command_que,
			d_inKeys,
			CL_TRUE, 0,
			sizeof(uint)  * nkeys,
			h_keys,
			0, NULL, NULL );


	if(status == CL_INVALID_COMMAND_QUEUE){
		printf("if command_queue is not a valid command-queue.1 \n");
	}else if(status == CL_INVALID_CONTEXT){
		printf("if command_queue is not a valid command-queue.2 \n");
	}else if(status == CL_INVALID_MEM_OBJECT){
		printf("if command_queue is not a valid command-queue.3 \n");
	}else if(status == CL_INVALID_VALUE){
		printf("if command_queue is not a valid command-queue.4 \n");
	}else if(status == CL_INVALID_EVENT_WAIT_LIST){
		printf("if command_queue is not a valid command-queue.5 \n");
	}else if(status == CL_MISALIGNED_SUB_BUFFER_OFFSET){
		printf("if command_queue is not a valid command-queue. 6\n");
	}else if(status == CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST){
		printf("if command_queue is not a valid command-queue.7 \n");
	}else if(status == CL_MEM_OBJECT_ALLOCATION_FAILURE){
		printf("if command_queue is not a valid command-queue.8 \n");
	}else if(status == CL_OUT_OF_RESOURCES){
		printf("if command_queue is not a valid command-queue. 9\n");
	}else if(status == CL_OUT_OF_HOST_MEMORY){
		printf("if command_queue is not a valid command-queue.10 \n");
	}

//CL_MEM_OBJECT_ALLOCATION_FAILURE
//	 if the context associated with command_queue and buffer are not the same or if the context associated with command_queue and events in event_wait_list are not the same.
//	 if buffer is not a valid buffer object.
//	 if the region being written specified by (offset, cb) is out of bounds or if ptr is a NULL value.
//	 if event_wait_list is NULL and num_events_in_wait_list greater than 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events.
//	 if buffer is a sub-buffer object and offset specified when the sub-buffer object is created is not aligned to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue.
//	 if the read and write operations are blocking and the execution status of any of the events in event_wait_list is a negative integer value.
//	 if there is a failure to allocate memory for data store associated with buffer.
//	 if there is a failure to allocate resources required by the OpenCL implementation on the device.

	assert (status == CL_SUCCESS);
	clFinish(command_que);  // wait end of read

	status = clEnqueueWriteBuffer( command_que,
			d_inPermut,
			CL_TRUE, 0,
			sizeof(uint)  * nkeys,
			h_Permut,
			0, NULL, NULL );

	assert (status == CL_SUCCESS);
	clFinish(command_que);  // wait end of read
}



// compute the histograms
void cl_radix_histogram(uint pass){

	cl_int err;

	size_t nblocitems=_ITEMS;
	size_t nbitems=_GROUPS*_ITEMS;

	assert(_RADIX == pow(2,_BITS));

	err  = clSetKernelArg(ckHistogram, 0, sizeof(cl_mem), &d_inKeys);
	assert(err == CL_SUCCESS);

	err = clSetKernelArg(ckHistogram, 2, sizeof(uint), &pass);
	assert(err == CL_SUCCESS);

	assert( nkeys_rounded%(_GROUPS * _ITEMS) == 0);
	assert( nkeys_rounded <= nkeys);

	err = clSetKernelArg(ckHistogram, 4, sizeof(uint), &nkeys_rounded);
	assert(err == CL_SUCCESS);

	cl_event eve;

	err = clEnqueueNDRangeKernel(command_que,
			ckHistogram,
			1, NULL,
			&nbitems,
			&nblocitems,
			0, NULL, &eve);

	//cout << err<<" , "<<CL_OUT_OF_RESOURCES<<endl;
	assert(err== CL_SUCCESS);

	clFinish(command_que);


	//////////////////////////////////////////////////
	// For profiling
	//////////////////////////////////////////////////
	cl_ulong debut,fin;
	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_QUEUED,
			sizeof(cl_ulong),
			(void*) &debut,
			NULL);
	assert(err== CL_SUCCESS);

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_END,
			sizeof(cl_ulong),
			(void*) &fin,
			NULL);
	assert(err== CL_SUCCESS);
	histo_time += (float) (fin-debut)/1e9;
	//////////////////////////////////////////////////



}

// scan the histograms
void cl_radix_scan_histogram(void){

	cl_int err;

	// numbers of processors for the local scan
	// half the size of the local histograms
	size_t nbitems=_RADIX* _GROUPS*_ITEMS / 2;


	size_t nblocitems= nbitems/_HISTOSPLIT ;


	int maxmemcache=max(_HISTOSPLIT,_ITEMS * _GROUPS * _RADIX / _HISTOSPLIT);

	// scan locally the histogram (the histogram is split into several
	// parts that fit into the local memory)

	err = clSetKernelArg(ckScanHistogram, 0, sizeof(cl_mem), &d_Histograms);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckScanHistogram, 1,
			sizeof(uint)* maxmemcache ,
			NULL); // mem cache

	err = clSetKernelArg(ckScanHistogram, 2, sizeof(cl_mem), &d_globsum);
	assert(err == CL_SUCCESS);

	cl_event eve;

	err = clEnqueueNDRangeKernel(command_que,
			ckScanHistogram,
			1, NULL,
			&nbitems,
			&nblocitems,
			0, NULL, &eve);

	// cout << err<<","<< CL_INVALID_WORK_ITEM_SIZE<< " "<<nbitems<<" "<<nblocitems<<endl;
	// cout <<CL_DEVICE_MAX_WORK_ITEM_SIZES<<endl;
	assert(err== CL_SUCCESS);
	clFinish(command_que);

	cl_ulong debut,fin;

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_QUEUED,
			sizeof(cl_ulong),
			(void*) &debut,
			NULL);
	//cout << err<<" , "<<CL_PROFILING_INFO_NOT_AVAILABLE<<endl;
	assert(err== CL_SUCCESS);

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_END,
			sizeof(cl_ulong),
			(void*) &fin,
			NULL);
	assert(err== CL_SUCCESS);

	scan_time += (float) (fin-debut)/1e9;

	// second scan for the globsum
	err = clSetKernelArg(ckScanHistogram, 0, sizeof(cl_mem), &d_globsum);
	assert(err == CL_SUCCESS);

	// err  = clSetKernelArg(ckScanHistogram, 1,
	// 			sizeof(uint)* _HISTOSPLIT,
	// 			NULL); // mem cache

	err = clSetKernelArg(ckScanHistogram, 2, sizeof(cl_mem), &d_temp);
	assert(err == CL_SUCCESS);

	nbitems= _HISTOSPLIT / 2;
	nblocitems=nbitems;
	//nblocitems=1;

	err = clEnqueueNDRangeKernel(command_que,
			ckScanHistogram,
			1, NULL,
			&nbitems,
			&nblocitems,
			0, NULL, &eve);

	assert(err== CL_SUCCESS);
	clFinish(command_que);

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_QUEUED,
			sizeof(cl_ulong),
			(void*) &debut,
			NULL);
	//cout << err<<" , "<<CL_PROFILING_INFO_NOT_AVAILABLE<<endl;
	assert(err== CL_SUCCESS);

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_END,
			sizeof(cl_ulong),
			(void*) &fin,
			NULL);
	assert(err== CL_SUCCESS);

	//  cout <<"durée global scan ="<<(float) (fin-debut)/1e9<<" s"<<endl;
	scan_time += (float) (fin-debut)/1e9;


	// loops again in order to paste together the local histograms
	nbitems = _RADIX* _GROUPS*_ITEMS/2;
	nblocitems=nbitems/_HISTOSPLIT;

	err = clEnqueueNDRangeKernel(command_que,
			ckPasteHistogram,
			1, NULL,
			&nbitems,
			&nblocitems,
			0, NULL, &eve);

	assert(err== CL_SUCCESS);
	clFinish(command_que);

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_QUEUED,
			sizeof(cl_ulong),
			(void*) &debut,
			NULL);
	//cout << err<<" , "<<CL_PROFILING_INFO_NOT_AVAILABLE<<endl;
	assert(err== CL_SUCCESS);

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_END,
			sizeof(cl_ulong),
			(void*) &fin,
			NULL);
	assert(err== CL_SUCCESS);

	//  cout <<"durée paste ="<<(float) (fin-debut)/1e9<<" s"<<endl;

	scan_time += (float) (fin-debut)/1e9;


}

// reorder the data from the scanned histogram
void cl_radix_reorder(uint pass){


	cl_int err;

	size_t nblocitems=_ITEMS;
	size_t nbitems=_GROUPS*_ITEMS;


	clFinish(command_que);

	err  = clSetKernelArg(ckReorder, 0, sizeof(cl_mem), &d_inKeys);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckReorder, 1, sizeof(cl_mem), &d_outKeys);
	assert(err == CL_SUCCESS);

	err = clSetKernelArg(ckReorder, 3, sizeof(uint), &pass);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckReorder, 4, sizeof(cl_mem), &d_inPermut);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckReorder, 5, sizeof(cl_mem), &d_outPermut);
	assert(err == CL_SUCCESS);

	err  = clSetKernelArg(ckReorder, 6,
			sizeof(uint)* _RADIX * _ITEMS ,
			NULL); // mem cache
	assert(err == CL_SUCCESS);

	assert( nkeys_rounded%(_GROUPS * _ITEMS) == 0);

	err = clSetKernelArg(ckReorder, 7, sizeof(uint), &nkeys_rounded);
	assert(err == CL_SUCCESS);


	assert(_RADIX == pow(2,_BITS));

	cl_event eve;

	err = clEnqueueNDRangeKernel(command_que,
			ckReorder,
			1, NULL,
			&nbitems,
			&nblocitems,
			0, NULL, &eve);

	//cout << err<<" , "<<CL_MEM_OBJECT_ALLOCATION_FAILURE<<endl;

	assert(err== CL_SUCCESS);
	clFinish(command_que);

	cl_ulong debut,fin;

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_QUEUED,
			sizeof(cl_ulong),
			(void*) &debut,
			NULL);
	//cout << err<<" , "<<CL_PROFILING_INFO_NOT_AVAILABLE<<endl;
	assert(err== CL_SUCCESS);

	err=clGetEventProfilingInfo (eve,
			CL_PROFILING_COMMAND_END,
			sizeof(cl_ulong),
			(void*) &fin,
			NULL);
	assert(err== CL_SUCCESS);

	//cout <<"durée="<<(float) (fin-debut)/1e9<<" s"<<endl;
	reorder_time += (float) (fin-debut)/1e9;



	// swap the old and new vectors of keys
	cl_mem d_temp;
	d_temp=d_inKeys;
	d_inKeys=d_outKeys;
	d_outKeys=d_temp;

	// swap the old and new permutations
	d_temp=d_inPermut;
	d_inPermut=d_outPermut;
	d_outPermut=d_temp;

}

//  van der corput sequence
float corput(int n,int k1,int k2){
	float corput=0;
	float s=1;
	while(n>0){
		s/=k1;
		corput+=(k2*n%k1)%k1*s;
		n/=k1;
	}
	return corput;
}















