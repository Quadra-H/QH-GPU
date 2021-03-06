/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 *
 */


#ifndef __QHGPU_DAEMON_H__
#define __QHGPU_DAEMON_H__




#if defined (__APPLE__) || defined(MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif




#define SERVICE_INIT "init_service"
#define SERVICE_FINIT "finit_service"
#define SERVICE_LIB_PREFIX "libsrv_"

typedef int (*fn_init_service)(
    void* libhandle, int (*reg_srv)(struct qhgpu_service *, void*),
	cl_context context,
	cl_device_id* devices);
typedef int (*fn_finit_service)(
    void* libhandle, int (*unreg_srv)(const char*));




#ifdef __QHGPU__

struct qhgpu_service * qc_lookup_service(const char *name);
int qc_register_service(struct kgpu_service *s, void *libhandle);
int qc_unregister_service(const char *name);
int qc_load_service(const char *libpath);
int qc_load_all_services(const char *libdir);
int qc_unload_service(const char *name);
int qc_unload_all_services();

#endif /* __QHGPU__ */

#endif
