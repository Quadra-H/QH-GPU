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


static int mmap_ioctl_callback(struct qhgpu_request *req) {
	printk("[mmap ioctl module]mmap_ioctl_callback\n");
	struct completion *c = (struct completion*)req->kdata;
	complete(c);

	return 0;
}


static int __init minit(void) {
	//max int data_size == 0x100000
	//max char data_size == 0x400000 == pagesize * 2^10
	const unsigned int DATA_SIZE = 0x100000;

	struct qhgpu_request *req;

	int* int_buf;
	int* mmap_addr;
	int i, j;

	struct timeval t0, t1;
	long tt;

	printk("[mmap ioctl module]init\n");

	//// alloc request
	req = qhgpu_alloc_request();
	if (!req) {
		printk("request null\n");
		return 0;
	}

	strcpy(req->service_name, "mmap_ioctl_service");
	req->callback = mmap_ioctl_callback;
	// init mmap_addr
	req->kmmap_addr = qhgpu_mmap_addr_pass();
	req->mmap_size = DATA_SIZE;

	//set mmap_addr
	mmap_addr = req->kmmap_addr;

	//4MB
	int_buf = (unsigned int *)__get_free_pages(GFP_KERNEL, 10);
	if(int_buf == NULL) {
		printk("[mmap ioctl module]int_buf __get_free_pages error.\n");
		return 1;
	}

	for(i = 0 ; i < DATA_SIZE ; i++)
		int_buf[i] = i;

	do_gettimeofday(&t0);
	memcpy(mmap_addr, int_buf, DATA_SIZE*sizeof(int));
	do_gettimeofday(&t1);

	for( i = 0 ; i < DATA_SIZE ; i++ ) {
		if(i != mmap_addr[i]) {
			printk("[mmap ioctl module]memcpy error in index [%x].\n", i);
			printk("[mmap ioctl module]mmap_addr dump\n");
			for( j = i - 16 ; j < i + 16 ; j++ )
				printk("[%x]", mmap_addr[i]);
			printk("[mmap ioctl module]mmap_addr dump end\n");
			break;
		}
	}

	tt = 1000000*(t1.tv_sec-t0.tv_sec) +
			((long)(t1.tv_usec) - (long)(t0.tv_usec));

	printk("[mmap ioctl module]TIME: %10lu MS\n", tt);

	qhgpu_call_sync(req);

	printk("[mmap ioctl module]init\n");

	//free_pages(buffer, order);
	free_pages(int_buf, 10);

	return 0;
}

static void __exit mexit(void) {
	printk("[mmap ioctl module]exit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
