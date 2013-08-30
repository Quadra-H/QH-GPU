
/* This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../qhgpu/qhgpu.h"
#include "../../../qhgpu/connector.h"
#include "cl_radix_sort.h"


#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif




char exten[1000]; // list of extensions to opencl language
cl_context context;
cl_device_id dev;


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Default Process
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

int batch_cs(struct qhgpu_service_request *sr)
{
	printf("[libsrv_default] Info: default_cs\n");
	return 0;
}


// launch and copy result to sr->dout
int batch_launch(struct qhgpu_service_request *sr)
{

	int i=0;


	printf("[libsrv_batch] Info: batch_launch !!!3\n");
	printf("mmap test!!!:  %p \n", sr->mmap_addr);

	unsigned int* mmap_data =sr->mmap_addr;//qhgpudev.mmap_private_data;
	printf("batch_launch mmap_data size : %d %d \n", sizeof(sr->mmap_addr),sr->mmap_size);
	int mmap_size = sr->mmap_size;
	//_MAX_BATCH_SIZE

	h_keys = (uint*)malloc(sizeof(uint)*mmap_size);


	//int *batch_cnt = (int*)sr->hin;
	printf("batch_cnt: %d \n",sr->batch_size);
	int offset = 0;
	for(i=0;i<sr->batch_size;i++){
		int cpy_size = sr->batch_size == (i+1) ? mmap_size%_MAX_BATCH_SIZE:_MAX_BATCH_SIZE	;
		if(cpy_size ==0 )cpy_size= _MAX_BATCH_SIZE;


		printf("batch cpy : %d \n",cpy_size);
		memcpy(h_keys+offset,sr->mmap_addr, cpy_size*sizeof(uint));
		offset+=cpy_size;

		if((i+1)!=sr->batch_size){
			int r = ioctl(sr->devfd, QHGPU_IOC_BATCH_H2D,(unsigned long)sr->id);
			if (r < 0) {
				printf("device write fail!");
				perror("device write fail!");
				abort();
			}
		}
	}
	//h_keys = sr->mmap_addr;
	printf("\n\n init batch =====\n");

	init_cl_radix_sort(sr->mmap_size);

	printf("Radix= %d \n",_RADIX);
	printf("Max Int= %d \n",(uint) _MAXINT);
	cl_radix_sort();	printf("\n\n recup =====\n");

	////////////////////////////////////////////////////////////////////////
	//read result
	////////////////////////////////////////////////////////////////////////
	cl_radix_recup_gpu();
	////////////////////////////////////////////////////////////////////////

	printf("\n\n");


	printf("%f s in the histograms\n",histo_time);
	printf("%f s in the scanning\n",scan_time);
	printf("%f s in the reordering\n",reorder_time);
	printf("%f s in the transposition\n",transpose_time);
	printf("%f s total GPU time (without memory transfers)\n",sort_time);



	//batch_cnt = (int*)sr->hin;
	offset = 0;
	for(i=0;i<sr->batch_size;i++){
		int cpy_size = sr->batch_size == (i+1) ? mmap_size%_MAX_BATCH_SIZE:_MAX_BATCH_SIZE	;
		if(cpy_size ==0 )cpy_size= _MAX_BATCH_SIZE;


		printf("batch cpy : %d \n",cpy_size);
		memcpy(sr->mmap_addr,h_keys+offset, cpy_size*sizeof(uint));
		offset+=cpy_size;

		//if((i+1)!=batch_cnt[0]){
		int r = ioctl(sr->devfd, QHGPU_IOC_BATCH_D2H,(unsigned long)sr->id);
		if (r < 0) {
			printf("device write fail!");
			perror("device write fail!");
			abort();
		}
		//}
	}


	return 0;
}
int batch_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_batch] Info: batch_post\n");
	return 0;
}


int batch_prepare(){
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////











static struct qhgpu_service batch_srv;

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*),
		cl_context ctx,
		cl_device_id* dv)

{
	printf("[libsrv_batch] Info: init batch service !!!!\n");

	context= ctx;
	dev  = dv;

	build_cl_radix_sort(context,dev);

	/////////////////////////////////////////////
	//batch sort init
	/////////////////////////////////////////////
	batch_prepare();
	/////////////////////////////////////////////


	sprintf(batch_srv.name, "batch_service");
	batch_srv.sid = 1;
	batch_srv.compute_size = batch_cs;
	batch_srv.launch = batch_launch;
	batch_srv.post = batch_post;


	return reg_srv(&batch_srv, lh);
}

int finit_service(void *lh, int (*unreg_srv)(const char*))
{
	printf("[libsrv_default] Info: finit test service\n");
	return unreg_srv(batch_srv.name);
}
