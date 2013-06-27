/*
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 *
 *
 * Weibin Sun
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/gfp.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/bitmap.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include "qhgpu.h"
#include <linux/types.h>

#define __KERNEL__

static int __init mod_init(void) {
	qhgpu_log(QH_LOG_INFO, "main", "mod_init", 1, "aaa", "bbb");
	//qhgpu_log(QHGPU_LOG_PRINT, "QHGPU loaded\n");
	return 1;
}

static void __exit mod_exit(void) {
	//qhgpu_log(QHGPU_LOG_PRINT, "QHGPU unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Quadra-H");
MODULE_DESCRIPTION("GPU computing framework for kernel");
