
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
#include "../../../qhgpu/global.h"
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

int radix_cs(struct qhgpu_service_request *sr)
{
	printf("[libsrv_default] Info: default_cs\n");
	return 0;
}


// launch and copy result to sr->dout
int radix_launch(struct qhgpu_service_request *sr)
{

	int i=0;


	printf("[libsrv_radix] Info: radix_launch !!!3\n");
	printf("mmap test!!!:  %p \n", sr->mmap_addr);

	unsigned int* mmap_data =sr->mmap_addr;//qhgpudev.mmap_private_data;
	printf("radix_launch mmap_data size : %d %d \n", sizeof(sr->mmap_addr),sr->mmap_size);

	h_keys = sr->mmap_addr;


	printf("\n\n init radix =====\n");

	//init_cl_radix_sort(Context,Devices[numdev],CommandQueue,sr->mmap_size);
	init_cl_radix_sort(sr->mmap_size);

	printf("Radix= %d \n",_RADIX);
	printf("Max Int= %d \n",(uint) _MAXINT);
	cl_radix_sort();

	printf("\n\n recup =====\n");

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

	return 0;
}
int radix_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_radix] Info: radix_post\n");
	return 0;
}


int radix_prepare(){



}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////











static struct qhgpu_service radix_srv;

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*),
		cl_context ctx,
		cl_device_id* dv)

{
	printf("[libsrv_radix] Info: init radix service !!!!\n");

	context= ctx;
	dev  = dv;




	build_cl_radix_sort(context,dev);

	/////////////////////////////////////////////
	//radix sort init
	/////////////////////////////////////////////
	radix_prepare();
	/////////////////////////////////////////////


	sprintf(radix_srv.name, "radix_service");
	radix_srv.sid = 1;
	radix_srv.compute_size = radix_cs;
	radix_srv.launch = radix_launch;
	radix_srv.post = radix_post;


	return reg_srv(&radix_srv, lh);
}

int finit_service(void *lh, int (*unreg_srv)(const char*))
{
	printf("[libsrv_default] Info: finit test service\n");
	return unreg_srv(radix_srv.name);
}
