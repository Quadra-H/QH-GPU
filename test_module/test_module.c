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

int test_data=777;


static int __init minit(void) {
	printk("test_module init\n");
	//user_call_test();

	int err=0;

	size_t nbytes;

	unsigned int cur;

	struct qhgpu_request *req;
	char *buf;


	buf = qhgpu_vmalloc(sizeof(int));
	if (!buf) {
		printk("GPU buffer is null.\n");
		return -EFAULT;
	}


	req  = qhgpu_alloc_request();
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



	/*


	memcpy(req->udata, &test_data, sizeof(test_data));
	strcpy(req->service_name, "test module");


	if (qhgpu_call_sync(req)) {
		err = -EFAULT;
		printk("callgpu error\n");
	} else {
		printk("callgpu success\n");
	}
	qhgpu_vfree(req->in);
	qhgpu_free_request(req);*/

	return 0;
}

static void __exit mexit(void) {
	printk("test_module exit\n");
	user_callback_test();
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
