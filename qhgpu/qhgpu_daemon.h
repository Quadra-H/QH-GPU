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

struct qhgpu_service {
    char name[QHGPU_SERVICE_NAME_SIZE];
    int sid;
    int (*compute_size)(struct qhgpu_service_request *sreq);
    int (*launch)(struct qhgpu_service_request *sreq);
    int (*prepare)(struct qhgpu_service_request *sreq);
    int (*post)(struct qhgpu_service_request *sreq);
};

#define SERVICE_INIT "init_service"
#define SERVICE_FINIT "finit_service"
#define SERVICE_LIB_PREFIX "libsrv_"

typedef int (*fn_init_service)(
    void* libhandle, int (*reg_srv)(struct kgpu_service *, void*));
typedef int (*fn_finit_service)(
    void* libhandle, int (*unreg_srv)(const char*));


#ifdef __QHGPU__
/*
struct kgpu_service * kh_lookup_service(const char *name);
int kh_register_service(struct kgpu_service *s, void *libhandle);
int kh_unregister_service(const char *name);
int kh_load_service(const char *libpath);
int kh_load_all_services(const char *libdir);
int kh_unload_service(const char *name);
int kh_unload_all_services();
*/
#endif /* __QHGPU__ */

#endif
