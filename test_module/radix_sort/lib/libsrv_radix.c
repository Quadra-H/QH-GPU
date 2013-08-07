
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


cl_device_id* Devices;   // tableau des devices

char exten[1000]; // list of extensions to opencl language



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
	printf("[libsrv_radix] Info: radix_launch !!!3\n");
	printf("mmap test!!!:  %p \n", sr->mmap_addr);

	unsigned int* mmap_data =sr->mmap_addr;//qhgpudev.mmap_private_data;
	printf("radix_launch mmap_data size : %d %d \n", sizeof(sr->mmap_addr),sr->mmap_size);

	h_keys = sr->mmap_addr;

	int i=0;
	for(i=0;i<50;i++){
		printf("%d ,", h_keys[i]);
	}

	init_cl_radix_sort(Context,Devices[numdev],CommandQueue,sr->mmap_size);

	printf("Radix= %d \n",_RADIX);
	printf("Max Int= %d \n",(uint) _MAXINT);
	cl_radix_sort();

	printf("\n\n");


	for(i=0;i<50;i++){
		printf("%d ,", h_keys[i]);
	}


	////////////////////////////////////////////////////////////////////////
	//read result
	////////////////////////////////////////////////////////////////////////
	cl_radix_recup_gpu();
	////////////////////////////////////////////////////////////////////////



	printf("\n\n");

	for(i=nkeys-60;i<nkeys-10;i++){
		printf("%d ,", h_keys[i]);
	}

	printf("%f s in the histograms\n",histo_time);
	printf("%f s in the scanning\n",scan_time);
	printf("%f s in the reordering\n",reorder_time);
	printf("%f s in the transposition\n",transpose_time);
	printf("%f s total GPU time (without memory transfers)\n",sort_time);

	// check the results (debugging)
	//cl_radix_check();



	/**/



	return 0;
}
int radix_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_radix] Info: radix_post\n");

	return 0;
}


int radix_prepare(){

	//cout <<endl<< "Test cl_radix_sort class..."<<endl<<endl;

	status = clGetPlatformIDs(0, NULL, &NbPlatforms);
	assert (status == CL_SUCCESS);
	assert(NbPlatforms > 0);
	//cout << "Found "<<NbPlatforms<<" OpenCL platform"<<endl;

	//cl_platform_id* Platforms = new cl_platform_id[NbPlatforms];
	cl_platform_id* Platforms =(cl_platform_id *) malloc ( NbPlatforms * sizeof(cl_platform_id) );

	status = clGetPlatformIDs(NbPlatforms, Platforms, NULL);
	assert (status == CL_SUCCESS);

	// affichage
	char pbuf[1000];
	int i=0;
	for ( i = 0; i < (int)NbPlatforms; ++i) {
		status = clGetPlatformInfo(Platforms[0],
				CL_PLATFORM_VENDOR,
				sizeof(pbuf),
				pbuf,
				NULL);
		assert (status == CL_SUCCESS);

		//cout << pbuf <<endl;
	}

	// affichage version opencl
	//cout << "The OpenCL version is"<<endl;

	for (i = 0; i < (int)NbPlatforms; ++i) {
		status = clGetPlatformInfo(Platforms[0],
				CL_PLATFORM_VERSION,
				sizeof(pbuf),
				pbuf,
				NULL);
		assert (status == CL_SUCCESS);

		//cout << pbuf <<endl;
	}
	// affichages divers
	for ( i = 0; i < (int)NbPlatforms; ++i) {
		status = clGetPlatformInfo(Platforms[0],
				CL_PLATFORM_NAME,
				sizeof(pbuf),
				pbuf,
				NULL);
		assert (status == CL_SUCCESS);

		//cout << pbuf <<endl;
	}

	// comptage du nombre de devices
	status = clGetDeviceIDs(Platforms[0],
			CL_DEVICE_TYPE_ALL,
			0,
			NULL,
			&NbDevices);
	assert (status == CL_SUCCESS);
	assert(NbDevices > 0);

	//cout <<"Found "<<NbDevices<< " OpenCL device"<<endl;

	// allocation du tableau des devices
	//Devices = new cl_device_id[NbDevices];
	Devices =(cl_device_id *) malloc ( NbDevices * sizeof(cl_device_id) );

	// choix du numéro de device
	// en général, le premier est le gpu
	// (mais pas toujours)
	numdev=0;
	assert(numdev < NbDevices);

	// remplissage du tableau des devices
	status = clGetDeviceIDs(Platforms[0],
			CL_DEVICE_TYPE_ALL,
			NbDevices,
			Devices,
			NULL);
	assert (status == CL_SUCCESS);


	// informations diverses

	// type du device
	status = clGetDeviceInfo(
			Devices[numdev],
			CL_DEVICE_TYPE,
			sizeof(cl_device_type),
			(void*)&DeviceType,
			NULL);
	assert (status == CL_SUCCESS);


	status = clGetDeviceInfo(
			Devices[numdev],
			CL_DEVICE_EXTENSIONS,
			sizeof(exten),
			exten,
			NULL);
	assert (status == CL_SUCCESS);

	//cout<<"OpenCL extensions for this device:"<<endl;
	//cout << exten<<endl<<endl;

	// type du device
	status = clGetDeviceInfo(
			Devices[numdev],
			CL_DEVICE_TYPE,
			sizeof(cl_device_type),
			(void*)&DeviceType,
			NULL);
	assert (status == CL_SUCCESS);

	if (DeviceType == CL_DEVICE_TYPE_CPU){
		//cout << "Calcul sur CPU"<<endl;
	}
	else{
		//cout << "Calcul sur Carte Graphique"<<endl;
	}

	// mémoire cache du  device
	cl_ulong memcache;
	status = clGetDeviceInfo(
			Devices[numdev],
			CL_DEVICE_LOCAL_MEM_SIZE,
			sizeof(cl_ulong),
			(void*)&memcache,
			NULL);
	assert (status == CL_SUCCESS);

	//cout << "GPU cache="<<memcache<<endl;
	//cout << "Needed cache="<< _ITEMS*_RADIX*sizeof(int)<<endl;

	// nombre de CL_DEVICE_MAX_COMPUTE_UNITS
	cl_int cores;
	status = clGetDeviceInfo(
			Devices[numdev],
			CL_DEVICE_MAX_COMPUTE_UNITS,
			sizeof(cl_int),
			(void*)&cores,
			NULL);
	assert (status == CL_SUCCESS);

	//cout << "Compute units="<<cores<<endl;

	// création d'un contexte opencl
	//cout <<"Create the context"<<endl;
	Context = clCreateContext(0,
			1,
			&Devices[numdev],
			NULL,
			NULL,
			&status);

	assert (status == CL_SUCCESS);
	////////////////////////////////////////////////////////////////////////
	/// eof create context
	////////////////////////////////////////////////////////////////////////

}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////











static struct qhgpu_service radix_srv;

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*))
{
	printf("[libsrv_radix] Info: init radix service !!!!\n");
	sprintf(radix_srv.name, "radix_service");





	radix_prepare();



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
