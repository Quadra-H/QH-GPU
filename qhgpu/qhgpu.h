/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Common header for userspace helper, kernel mode KGPU and KGPU clients
 *
 */

#ifndef __QHGPU_H__
#define __QHGPU_H__



#define TO_UL(v) ((unsigned long)(v))

#define ADDR_WITHIN(pointer, base, size)		\
    (TO_UL(pointer) >= TO_UL(base) &&			\
     (TO_UL(pointer) < TO_UL(base)+TO_UL(size)))

#define ADDR_REBASE(dst_base, src_base, pointer)			\
    (TO_UL(dst_base) + (						\
	TO_UL(pointer)-TO_UL(src_base)))

struct qhgpu_gpu_mem_info {
    void *uva;
    unsigned long size;
};

#define QHGPU_SERVICE_NAME_SIZE 32
#include "qhgpu_daemon.h"


struct qhgpu_ku_request {
    int id;
    char service_name[QHGPU_SERVICE_NAME_SIZE];
    void *in, *out, *data;
    unsigned long insize, outsize, datasize;
};

/* qhgpu's errno */
#define QHGPU_OK 0
#define QHGPU_NO_RESPONSE 1
#define QHGPU_NO_SERVICE 2
#define QHGPU_TERMINATED 3

struct qhgpu_ku_response {
    int id;
    int errcode;
};

/*
 * Only for kernel code or helper
 */
//#if defined __KERNEL__ || defined __QHGPU__

/* the NR will not be used */
#define QHGPU_BUF_NR 1
#define QHGPU_BUF_SIZE (100*1024)

#define QHGPU_MMAP_SIZE KGPU_BUF_SIZE

#define QHGPU_DEV_NAME "qhgpu"

/* ioctl */
#include <linux/ioctl.h>

#define QHGPU_IOC_MAGIC 'g'

#define QHGPU_IOC_SET_GPU_BUFS \
    _IOW(QHGPU_IOC_MAGIC, 1, struct qhgpu_gpu_mem_info[QHGPU_BUF_NR])
#define QHGPU_IOC_GET_GPU_BUFS \
    _IOR(QHGPU_IOC_MAGIC, 2, struct qhgpu_gpu_mem_info[QHGPU_BUF_NR])
#define QHGPU_IOC_SET_STOP     _IO(QHGPU_IOC_MAGIC, 3)
#define QHGPU_IOC_GET_REQS     _IOR(QHGPU_IOC_MAGIkC, 4,

#define QHGPU_IOC_MAXNR 4

#include "qhgpu_log.h"





//#endif /* __KERNEL__ || __QHGPU__  */

/*
 * For helper and service providers
 */
//#ifndef __KERNEL__

struct qhgpu_service;

struct qhgpu_service_request {
    int id;
    void *hin, *hout, *hdata;
    void *din, *dout, *ddata;
    unsigned long insize, outsize, datasize;
    int errcode;
    struct qhgpu_service *s;
    int block_x, block_y;
    int grid_x, grid_y;
    int state;
    int stream_id;
    unsigned long stream;
};

/* service request states: */
#define QHGPU_REQ_INIT 1
#define QHGPU_REQ_MEM_DONE 2
#define QHGPU_REQ_PREPARED 3
#define QHGPU_REQ_RUNNING 4
#define QHGPU_REQ_POST_EXEC 5
#define QHGPU_REQ_DONE 6



//#endif /* no __KERNEL__ */

/*
 * For kernel code only
 */
#ifdef __KERNEL__

#include <linux/list.h>

struct qhgpu_request;

extern int user_call_test();
extern int user_callback_test();
extern void qhgpu_vfree(void *p);
extern void* qhgpu_vmalloc(unsigned long nbytes);
extern int qhgpu_call_sync(struct qhgpu_request *req);
extern struct qhgpu_request* qhgpu_alloc_request(void);
extern void qhgpu_free_request(struct qhgpu_request* req);


typedef int (*qhgpu_callback)(struct qhgpu_request *req);

struct qhgpu_request {
    int id;
    void *in, *out, *udata, *kdata;
    unsigned long insize, outsize, udatasize, kdatasize;
    char service_name[QHGPU_SERVICE_NAME_SIZE];
    qhgpu_callback callback;
    int errcode;
};
/*
extern int kgpu_call_sync(struct kgpu_request*);
extern int kgpu_call_async(struct kgpu_request*);

extern int kgpu_next_request_id(void);
extern struct kgpu_request* kgpu_alloc_request(void);
extern void kgpu_free_request(struct kgpu_request*);

extern void *kgpu_vmalloc(unsigned long nbytes);
extern void kgpu_vfree(void* p);

extern void *kgpu_map_pfns(unsigned long *pfns, int n);
extern void *kgpu_map_pages(struct page **pages, int n);
extern void kgpu_unmap_area(unsigned long addr);
extern int kgpu_map_page(struct page*, unsigned long);
extern void kgpu_free_mmap_area(unsigned long);
extern unsigned long kgpu_alloc_mmap_area(unsigned long);
*/
#endif /* __KERNEL__ */

#endif
