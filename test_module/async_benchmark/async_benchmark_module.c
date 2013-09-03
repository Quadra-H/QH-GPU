/* This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/gfp.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <asm/page.h>
#include <linux/timex.h>

#include "../../qhgpu/qhgpu.h"



#define BATCH_NR 10
struct completion cs[BATCH_NR];


int done =0;
static int async_gpu_callback(struct qhgpu_request *req)
{
	done++;
	//printk("async_gpu_callback %d\n",done);

	if(done == BATCH_NR){
		if(req->kdata!=NULL){
			struct completion *c = (struct completion*)req->kdata;
			complete(c);
		}
	}
	return 0;
}


static int sync_gpu_callback(struct qhgpu_request *req)
{
	done++;
	//printk("sync_gpu_callback\n");
	return 0;
}




static int __init minit(void) {
	printk("async_benchmark_module init\n");


	int err=0;

	size_t nbytes;
	unsigned int cur;
	unsigned long sz;
	struct timeval t0, t1;
	long tt_async, tt_sync ,start,end;
	int i=0;


	struct qhgpu_request *req;
	char *buf;

	//// alloc request
	req = qhgpu_alloc_request(1<<10,"default_service");
	if (!req) {
		printk("request null\n");
		return 0;
	}
	//////////////////////////////////////////////////////
	// init mmap_addr
	//////////////////////////////////////////////////////


	req->callback = async_gpu_callback;
	init_completion(cs);
	req->kdata = (void*)(cs);
	req->kdatasize = sizeof(void*);


	done= 0;
	do_gettimeofday(&t0);
	///////////////////////////////////////////////////
	for(i=0;i<BATCH_NR;i++)
		qhgpu_call_async(req);

	wait_for_completion(cs);
	///////////////////////////////////////////////////
	do_gettimeofday(&t1);

	tt_async = 1000000*(t1.tv_sec-t0.tv_sec) +
			((long)(t1.tv_usec) - (long)(t0.tv_usec));





	req->callback = sync_gpu_callback;
	do_gettimeofday(&t0);
	///////////////////////////////////////////////////
	for(i=0;i<BATCH_NR;i++)
		qhgpu_call_sync(req);
	///////////////////////////////////////////////////
	do_gettimeofday(&t1);

	tt_sync = 1000000*(t1.tv_sec-t0.tv_sec) +
			((long)(t1.tv_usec) - (long)(t0.tv_usec));

	printk("ASYNC TIME: %10lu MS, OPS: %8lu, BW: %8lu MB/S\n",tt_async, 1000000/tt_async, sz/tt_async);
	printk("SYNC TIME: %10lu MS, OPS: %8lu, BW: %8lu MB/S\n",tt_sync, 1000000/tt_sync, sz/tt_sync);





	return 0;
}

static void __exit mexit(void) {

	printk("async_benchmark_module exit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
