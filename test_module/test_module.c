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
#include <asm/page.h>

#include "../qhgpu/qhgpu.h"



#define BATCH_NR 3
#define MAX_MEM_SZ (1024)
#define MIN_MEM_SZ (128)



// mapped memory address
static char* mmap_addr;
const int ARRAY_SIZE = 1000;
static float* result;
static float* a;
static float* b;


static int test_gpu_callback(struct qhgpu_request *req)
{

	printk("test_gpu_callback\n");
	if(req->out!=NULL){
		int* out_arr = (int*)req->out;
		int i=0;
		char buffer [50];
		for(i=0;i<5;i++){

			sprintf (buffer, "test_out 21 : %d ~~~~~ \n", out_arr[i]);
			printk("%s",buffer);
		}
	}
	struct completion *c = (struct completion*)req->kdata;
	complete(c);


	return 0;
}



static int __init minit(void) {
	printk("test_module init\n");

	/*struct qhgpu_request *rs[BATCH_NR];
	void *bufs[BATCH_NR];
	struct completion cs[BATCH_NR];

	int i;
	struct timeval t0, t1;
	long tt;
	unsigned long sz;

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
		rs[i]->callback = test_gpu_callback;
		init_completion(cs+i);
		rs[i]->kdata = (void*)(cs+i);
		rs[i]->kdatasize = sizeof(void*);
		strcpy(rs[i]->service_name, "default_service");
		rs[i]->insize = PAGE_SIZE;
		rs[i]->outsize = PAGE_SIZE;
		rs[i]->udata = NULL;
		rs[i]->udatasize = 0;
	}

	printk("done allocations, start first test\n");

	qhgpu_call_sync(rs[0]);

	printk("done first test for CUDA init\n");

	rs[0]->id = qhgpu_next_request_id();

	for (sz=MIN_MEM_SZ; sz<=MAX_MEM_SZ; sz=(sz?sz<<1:PAGE_SIZE)) {
		for (i=0; i<BATCH_NR; i++) {
			rs[i]->insize = sz;
			rs[i]->outsize = sz;
		}

		do_gettimeofday(&t0);
		for (i=0; i<BATCH_NR; i++) {
			qhgpu_call_async(rs[i]);
		}

		for (i=0; i<BATCH_NR; i++)
			wait_for_completion(cs+i);
		do_gettimeofday(&t1);

		tt = 1000000*(t1.tv_sec-t0.tv_sec) +
				((long)(t1.tv_usec) - (long)(t0.tv_usec));
		tt /= BATCH_NR;

		printk("ASYNC SIZE: %10lu B, TIME: %10lu MS, OPS: %8lu, BW: %8lu MB/S\n",
				sz, tt, 1000000/tt, sz/tt);

		for (i=0; i<BATCH_NR; i++) {
			init_completion(cs+i);
			rs[i]->id = qhgpu_next_request_id();
		}
	}

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


	cleanup:
		for (i=0; i<BATCH_NR; i++) {
			if (rs[i]) qhgpu_free_request(rs[i]);
			if (bufs[i]) qhgpu_vfree(bufs[i]);
		}



	printk("done sync\n");
	*/



	struct completion cs[BATCH_NR];

	int err=0;

	size_t nbytes;
	unsigned int cur;

	struct qhgpu_request *req;
	char *buf;

	//// alloc request
	req = qhgpu_alloc_request();
	if (!req) {
		printk("request null\n");
		return 0;
	}

	a = ( float * ) kmalloc( ARRAY_SIZE * ( sizeof( float ) ), GFP_KERNEL);
	b = ( float * ) kmalloc( ARRAY_SIZE * ( sizeof( float ) ), GFP_KERNEL);

	int i=0;
	for (i = 1; i < ARRAY_SIZE; i++)
	{
		a[i] = (float)i;
		b[i] = (float)(i * 2);
	}

	printk("test_module var init\n");

	buf = qhgpu_vmalloc((ARRAY_SIZE * ( sizeof( float ))));
	if (!buf) {
		printk("GPU buffer is null.\n");
		return -EFAULT;
	}

	req->in = buf;																			// for hin
	req->out = (void*)((unsigned long)(req->in)+(ARRAY_SIZE * ( sizeof( int ))));	// for hout
	req->insize = sizeof(buf);
	req->outsize = sizeof((ARRAY_SIZE * ( sizeof( int ))));
	strcpy(req->service_name, "default_service");
	req->udatasize = sizeof(result[ARRAY_SIZE]);
	req->udata = buf;																			// for hdata
	req->callback = test_gpu_callback;



	// init mmap_addr
	mmap_addr = qhgpu_mmap_addr_pass();
	memcpy(mmap_addr, "mmap_test!", 10);


	memcpy(req->in, b, (ARRAY_SIZE-5) * ( sizeof( float ) ));			/// for test

	qhgpu_call_sync(req);
	/**/

	if(req->out!=NULL){
		int* out_arr = (int*)req->out;
		int i=0;
		char buffer [50];
		for(i=0;i<5;i++){
			sprintf (buffer, "test_out 21 : %d ~~~~~ \n", out_arr[i]);
			printk("%s",buffer);
		}
	}
	return 0;
}

static void __exit mexit(void) {
	printk("test_module exit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
