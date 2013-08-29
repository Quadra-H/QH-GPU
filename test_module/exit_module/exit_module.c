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


static int exit_module_callback(struct qhgpu_request *req) {
	printk("[exit module]exit_module_callback\n");
	struct completion *c = (struct completion*)req->kdata;
	complete(c);

	return 0;
}


static int __init minit(void) {
	struct qhgpu_request *req;

	int* int_buf;
	int* mmap_addr;
	int i, j;

	int req_id;

	char* cbuf;

	printk("[exit module]init\n");

	//// alloc request
	req = qhgpu_alloc_request();
	if (!req) {
		printk("request null\n");
		return 0;
	}

	strcpy(req->service_name, "000 exit_module");
	req->callback = exit_module_callback;
	// init mmap_addr
	req_id = qhgpu_mmap_id();
	cbuf = qhgpu_vmalloc(sizeof( char )*3);
	sprintf (cbuf, "%d", req_id);
	req->udata= cbuf;
	printk("----%d , %c\n", req_id, cbuf[0]);
	req->kmmap_addr = qhgpu_mmap_addr_pass(req_id);
	req->mmap_size = 0;

	//set mmap_addr
	mmap_addr = req->kmmap_addr;

	qhgpu_call_sync(req);

	return 0;
}

static void __exit mexit(void) {
	printk("[exit module]exit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
