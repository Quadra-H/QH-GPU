/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2013 Quadra-H.
 * All rights reserved.
 *
 */

#ifndef __CONNECTOR_H__
#define __CONNECTOR_H__

#include <assert.h>
#include "qhgpu.h"
#include "list.h"

#if defined (__APPLE__) || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

//#define QC_LOG

struct qhgpu_service {
	char name[QHGPU_SERVICE_NAME_SIZE];
	int sid;
	int (*compute_size)(struct qhgpu_service_request *sreq);
	int (*prepare)(struct qhgpu_service_request *sreq, cl_context GPUContext,
			cl_device_id dev, cl_command_queue CommandQue);
	int (*launch)(struct qhgpu_service_request *sreq);
	int (*post)(struct qhgpu_service_request *sreq);
};

struct _qhgpu_sitem {
	struct qhgpu_service *s;
	void* libhandle;
	struct list_head list;
};

#ifndef PAGE_SIZE
#define PAGE_SIZE (1024*1024*2)
#endif

#endif
