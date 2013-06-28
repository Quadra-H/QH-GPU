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

static int __init minit(void) {
	printk("test_module init\n");
	user_call_test();
	return 0;
}

static void __exit mexit(void) {
	printk("test_module exit\n");
	user_callback_test();
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
