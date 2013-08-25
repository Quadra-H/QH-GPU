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


// Default Process
int mmap_ioctl_cs(struct qhgpu_service_request *sr)
{
	printf("[libsrv_default] Info: mmap_ioctl_cs\n");
	return 0;
}


// launch and copy result to sr->dout
int mmap_ioctl_launch(struct qhgpu_service_request *sr)
{
	int i=0;
	int* mmap_data;
	unsigned int mmap_size;

	printf("[libsrv_mmap_ioctl]mmap_ioctl launch\n");

	mmap_data = sr->mmap_addr;
	mmap_size = sr->mmap_size;

	for(i = mmap_size - 60 ; i < mmap_size ; i++){
		printf("%d ,", h_keys[i]);
	}

	return 0;
}

int mmap_ioctl_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_mmap_ioctl] Info: mmap_ioctl_post\n");
	return 0;
}

//set service
static struct qhgpu_service mmap_ioctl_srv;

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*))
{
	printf("[libsrv_mmap_ioctl] Info: mmap_ioctl_service\n");

	sprintf(radix_srv.name, "mmap_ioctl_service");
	radix_srv.sid = 1;
	radix_srv.compute_size = mmap_ioctl_cs;
	radix_srv.launch = mmap_ioctl_launch;
	radix_srv.post = mmap_ioctl_post;

	return reg_srv(&radix_srv, lh);
}

int finit_service(void *lh, int (*unreg_srv)(const char*))
{
	printf("[libsrv_default] Info: mmap_ioctl_test finit\n");
	return unreg_srv(radix_srv.name);
}
