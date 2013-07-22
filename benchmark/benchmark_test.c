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
#include <linux/completion.h>
#include <linux/uaccess.h>
#include <asm/page.h>
#include <linux/timex.h>

#include "../qhgpu/qhgpu.h"

/* customized log function */
#define MAX_MEM_SZ (1*256)
#define MIN_MEM_SZ (0)

#define BATCH_NR 2

int mycb(struct qhgpu_request *req)
{
	printk("mycb callback\n");
	//struct completion *c = (struct completion*)req->kdata;
	//complete(c);
	return 0;
}



int test_data=777;

static int test_gpu_callback(struct qhgpu_request *req)b
{
	int *data = (int *)req->udata;
	printk("test_gpu_callback: %d\n",*data);
	return 0;
}

static int __init minit(void)
{
	struct qhgpu_request *rs[BATCH_NR];
	void *bufs[BATCH_NR];
	struct completion cs[BATCH_NR];

	int i;
	struct timeval; //t0, t1;
//	long tt;
//	unsigned long sz;

	memset(rs, 0, sizeof(struct qhgpu_request*)*BATCH_NR);
	memset(bufs, 0, sizeof(void*)*BATCH_NR);

	printk("prepare for testing\n");

	for (i=0; i<BATCH_NR; i++) {

		rs[i] = qhgpu_alloc_request();
		if (!rs[i]) {
			printk("request %d null\n", i);
			goto cleanup;
		}

		bufs[i] = qhgpu_vmalloc(MAX_MEM_SZ);
		if (!bufs[i]) {
			printk("buf %d null\n", i);
			goto cleanup;
		}
		rs[i]->in = bufs[i];
		rs[i]->out = bufs[i];
		rs[i]->callback = mycb;
		init_completion(cs+i);
		rs[i]->kdata = (void*)(cs+i);
		rs[i]->kdatasize = sizeof(void*);
		strcpy(rs[i]->service_name, "empty_service");
		rs[i]->insize = PAGE_SIZE;
		rs[i]->outsize = PAGE_SIZE;
		rs[i]->udata = NULL;
		rs[i]->udatasize = 0;
	}

	printk("done allocations, start first test\n");




	/*char *buf = qhgpu_vmalloc(sizeof(int));
	struct qhgpu_request *req  = qhgpu_alloc_request();
	if (!req) {
		qhgpu_vfree(buf);
		printk("can't allocate request\n");
		return -EFAULT;
	}
	req->in = buf;
	req->out = buf;
	req->insize = sizeof(int);
	req->outsize = sizeof(int);
	req->udatasize = sizeof(int);
	req->udata = buf;
	req->callback = test_gpu_callback;


	printk("data copy start \n");
	memcpy(req->udata, &test_data, sizeof(test_data));
	strcpy(req->service_name, "test module");
	printk("data copy end \n");
	qhgpu_call_sync(req);
	*/

	qhgpu_call_sync(rs[0]);

	printk("done first test for OpenCL init\n");

	/*rs[0]->id = qhgpu_next_request_id();


	printk("done async, start sync\n");
	for (sz=MIN_MEM_SZ; sz<=MAX_MEM_SZ; sz=(sz?sz<<1:PAGE_SIZE)) {
		rs[0]->insize = sz;
		rs[0]->outsize = sz;

		do_gettimeofday(&t0);
		qhgpu_call_sync(rs[0]);
		do_gettimeofday(&t1);

		tt = 1000000*(t1.tv_sec-t0.tv_sec) +
				((long)(t1.tv_usec) - (long)(t0.tv_usec));

		printk("SYNC  SIZE: %10lu B, TIME: %10lu MS, OPS: %8lu, BW: %8lu MB/S\n",
				sz, tt, 1000000/tt, sz/tt);
		rs[0]->id = qhgpu_next_request_id();
	}
	printk("done sync\n");*/
	/**/


cleanup:
	for (i=0; i<BATCH_NR; i++) {
		if (rs[i]) qhgpu_free_request(rs[i]);
		if (bufs[i]) qhgpu_vfree(bufs[i]);
	}


	printk("done test\n");
	return 0;
}

static void __exit mexit(void)
{
    printk("unload\n");
}

module_init(minit);
module_exit(mexit);

MODULE_LICENSE("GPL");
