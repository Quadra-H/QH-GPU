
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

	return 0;
}


extern "C" int default_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_default] Info: default_post\n");

	return 0;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////




static struct qhgpu_service default_srv;

extern "C" int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*),
		cl_context ctx,
		cl_device_id* dv)
{
	printf("[libsrv_default] Info: libsrv_default service !!!!\n");
	sprintf(default_srv.name, "default_service");

	default_srv.launch = default_launch;
	default_srv.post = default_post;


	return reg_srv(&default_srv, lh);
}

extern "C" int finit_service(void *lh, int (*unreg_srv)(const char*))
{
	printf("[libsrv_default] Info: finit default service\n");
	return unreg_srv(default_srv.name);
}
