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

struct _qhgpu_dev {
	struct cdev cdev;
	struct class *cls;
	dev_t devno;

	int state;
};

static struct _qhgpu_dev qhgpudev;
static atomic_t qhgpu_av = ATOMIC_INIT(1);

int qhgpu_open(struct inode *inode, struct file *filp) {
	printk("open called\n");

	if (!atomic_dec_and_test(&qhgpu_av)) {
		atomic_inc(&qhgpu_av);
		return -1;
	}

	filp->private_data = &qhgpudev;
	return 0;
}

ssize_t qhgpu_read(struct file *filp, char __user *buf, size_t c, loff_t *fpos) {
	//printk("qhgpu_read called\n");
	memcpy(buf, &qhgpudev.state, sizeof(int));

	return 4;
}

ssize_t qhgpu_write(struct file *filp, const char __user *buf, size_t count, loff_t *fpos) {
	printk("qhgpu_write called\n");
	memcpy(&qhgpudev.state, buf, sizeof(int));

	return 4;
}

static struct file_operations qhgpu_ops = {
		.owner 	= THIS_MODULE,
		.open 		= qhgpu_open,
		.read 		= qhgpu_read,
		.write 	= qhgpu_write
};

static int qhgpu_init(void) {
	int result = 0;
	int devno;

	/* dev class */
	qhgpudev.cls = class_create(THIS_MODULE, "QHGPU_DEV_NAME");
	if (IS_ERR(qhgpudev.cls)) {
		result = PTR_ERR(qhgpudev.cls);
		printk("can't create dev class for qhgpu\n");
		return result;
	}

	/* alloc dev */
	result = alloc_chrdev_region(&qhgpudev.devno, 0, 1, QHGPU_DEV_NAME);
	devno = MAJOR(qhgpudev.devno);
	qhgpudev.devno = MKDEV(devno, 0);

	if (result < 0) {
		printk("can't get major\n");
	} else {
		struct device *device;
		memset(&qhgpudev.cdev, 0, sizeof(struct cdev));
		cdev_init(&qhgpudev.cdev, &qhgpu_ops);
		qhgpudev.cdev.owner = THIS_MODULE;
		qhgpudev.cdev.ops = &qhgpu_ops;
		result = cdev_add(&qhgpudev.cdev, qhgpudev.devno, 1);
		if (result) {
			printk("can't add device %d", result);
		}

		/* create /dev/qhgpu */
		device = device_create(qhgpudev.cls, NULL, qhgpudev.devno, NULL,
				QHGPU_DEV_NAME);
		if (IS_ERR(device)) {
			printk("creating device failed\n");
			result = PTR_ERR(device);
		}
	}

	return result;
}

static int qhgpu_clean(void) {
    device_destroy(qhgpudev.cls, qhgpudev.devno);
    cdev_del(&qhgpudev.cdev);
    class_destroy(qhgpudev.cls);

    unregister_chrdev_region(qhgpudev.devno, 1);

	return 1;
}

int user_call_test() {
	printk("user_call_test called\n");
	qhgpudev.state = 1;

	return 1;
}
EXPORT_SYMBOL_GPL(user_call_test);

int user_callback_test() {
	printk("user_callback_test called\n");
	qhgpudev.state = 0;

	return 1;
}
EXPORT_SYMBOL_GPL(user_callback_test);

static int __init mod_init(void) {
	printk("init_mod\n");
	qhgpu_init();
	//qhgpu_log(QH_LOG_INFO, "main", "mod_init", 1, "aaa");
	return 0;
}

static void __exit mod_exit(void) {
	printk("exit_mod\n");
	qhgpu_clean();
	//qhgpu_log(QHGPU_LOG_PRINT, "QHGPU unloaded\n");
}

module_init( mod_init);
module_exit( mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Quadra-H");
MODULE_DESCRIPTION("GPU computing framework for kernel");
