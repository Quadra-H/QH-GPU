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





static int qhgpu_init(void)
{
    int result = 0;
    int devno;

    kgpudev.state = KGPU_OK;

    INIT_LIST_HEAD(&(kgpudev.reqs));
    INIT_LIST_HEAD(&(kgpudev.rtdreqs));

    spin_lock_init(&(kgpudev.reqlock));
    spin_lock_init(&(kgpudev.rtdreqlock));

    init_waitqueue_head(&(kgpudev.reqq));

    spin_lock_init(&(kgpudev.ridlock));
    spin_lock_init(&(kgpudev.gmpool_lock));
    spin_lock_init(&(kgpudev.vm_lock));

    memset(&kgpudev.vm, 0, sizeof(struct _kgpu_vma));

    kgpudev.rid_sequence = 0;

    kgpu_request_cache = kmem_cache_create(
	"kgpu_request_cache", sizeof(struct kgpu_request), 0,
	SLAB_HWCACHE_ALIGN, kgpu_request_constructor);
    if (!kgpu_request_cache) {
	kgpu_log(KGPU_LOG_ERROR, "can't create request cache\n");
	return -EFAULT;
    }

    kgpu_request_item_cache = kmem_cache_create(
	"kgpu_request_item_cache", sizeof(struct _kgpu_request_item), 0,
	SLAB_HWCACHE_ALIGN, kgpu_request_item_constructor);
    if (!kgpu_request_item_cache) {
	kgpu_log(KGPU_LOG_ERROR, "can't create request item cache\n");
	kmem_cache_destroy(kgpu_request_cache);
	return -EFAULT;
    }

    kgpu_sync_call_data_cache = kmem_cache_create(
	"kgpu_sync_call_data_cache", sizeof(struct _kgpu_sync_call_data), 0,
	SLAB_HWCACHE_ALIGN, kgpu_sync_call_data_constructor);
    if (!kgpu_sync_call_data_cache) {
	kgpu_log(KGPU_LOG_ERROR, "can't create sync call data cache\n");
	kmem_cache_destroy(kgpu_request_cache);
	kmem_cache_destroy(kgpu_request_item_cache);
	return -EFAULT;
    }


    /* initialize buffer info */
    memset(&kgpudev.gmpool, 0, sizeof(struct _kgpu_mempool));

    /* dev class */
    kgpudev.cls = class_create(THIS_MODULE, "KGPU_DEV_NAME");
    if (IS_ERR(kgpudev.cls)) {
	result = PTR_ERR(kgpudev.cls);
	kgpu_log(KGPU_LOG_ERROR, "can't create dev class for KGPU\n");
	return result;
    }

    /* alloc dev */
    result = alloc_chrdev_region(&kgpudev.devno, 0, 1, KGPU_DEV_NAME);
    devno = MAJOR(kgpudev.devno);
    kgpudev.devno = MKDEV(devno, 0);

    if (result < 0) {
        kgpu_log(KGPU_LOG_ERROR, "can't get major\n");
    } else {
	struct device *device;
	memset(&kgpudev.cdev, 0, sizeof(struct cdev));
	cdev_init(&kgpudev.cdev, &kgpu_ops);
	kgpudev.cdev.owner = THIS_MODULE;
	kgpudev.cdev.ops = &kgpu_ops;
	result = cdev_add(&kgpudev.cdev, kgpudev.devno, 1);
	if (result) {
	    kgpu_log(KGPU_LOG_ERROR, "can't add device %d", result);
	}

	/* create /dev/kgpu */
	device = device_create(kgpudev.cls, NULL, kgpudev.devno, NULL,
			       KGPU_DEV_NAME);
	if (IS_ERR(device)) {
	    kgpu_log(KGPU_LOG_ERROR, "creating device failed\n");
	    result = PTR_ERR(device);
	}
    }

    return result;
}

static void kgpu_cleanup(void)
{
    kgpudev.state = KGPU_TERMINATED;

    device_destroy(kgpudev.cls, kgpudev.devno);
    cdev_del(&kgpudev.cdev);
    class_destroy(kgpudev.cls);

    unregister_chrdev_region(kgpudev.devno, 1);
    if (kgpu_request_cache)
	kmem_cache_destroy(kgpu_request_cache);
    if (kgpu_request_item_cache)static int kgpu_init(void)
    {
        int result = 0;
        int devno;

        kgpudev.state = KGPU_OK;

        INIT_LIST_HEAD(&(kgpudev.reqs));
        INIT_LIST_HEAD(&(kgpudev.rtdreqs));

        spin_lock_init(&(kgpudev.reqlock));
        spin_lock_init(&(kgpudev.rtdreqlock));

        init_waitqueue_head(&(kgpudev.reqq));

        spin_lock_init(&(kgpudev.ridlock));
        spin_lock_init(&(kgpudev.gmpool_lock));
        spin_lock_init(&(kgpudev.vm_lock));

        memset(&kgpudev.vm, 0, sizeof(struct _kgpu_vma));

        kgpudev.rid_sequence = 0;

        kgpu_request_cache = kmem_cache_create(
    	"kgpu_request_cache", sizeof(struct kgpu_request), 0,
    	SLAB_HWCACHE_ALIGN, kgpu_request_constructor);
        if (!kgpu_request_cache) {
    	kgpu_log(KGPU_LOG_ERROR, "can't create request cache\n");
    	return -EFAULT;
        }

        kgpu_request_item_cache = kmem_cache_create(
    	"kgpu_request_item_cache", sizeof(struct _kgpu_request_item), 0,
    	SLAB_HWCACHE_ALIGN, kgpu_request_item_constructor);
        if (!kgpu_request_item_cache) {
    	kgpu_log(KGPU_LOG_ERROR, "can't create request item cache\n");
    	kmem_cache_destroy(kgpu_request_cache);
    	return -EFAULT;
        }

        kgpu_sync_call_data_cache = kmem_cache_create(
    	"kgpu_sync_call_data_cache", sizeof(struct _kgpu_sync_call_data), 0,
    	SLAB_HWCACHE_ALIGN, kgpu_sync_call_data_constructor);
        if (!kgpu_sync_call_data_cache) {
    	kgpu_log(KGPU_LOG_ERROR, "can't create sync call data cache\n");
    	kmem_cache_destroy(kgpu_request_cache);
    	kmem_cache_destroy(kgpu_request_item_cache);
    	return -EFAULT;
        }


        /* initialize buffer info */
        memset(&kgpudev.gmpool, 0, sizeof(struct _kgpu_mempool));

        /* dev class */
        kgpudev.cls = class_create(THIS_MODULE, "KGPU_DEV_NAME");
        if (IS_ERR(kgpudev.cls)) {static int kgpu_init(void)
        {
            int result = 0;
            int devno;

            kgpudev.state = KGPU_OK;

            INIT_LIST_HEAD(&(kgpudev.reqs));
            INIT_LIST_HEAD(&(kgpudev.rtdreqs));

            spin_lock_init(&(kgpudev.reqlock));
            spin_lock_init(&(kgpudev.rtdreqlock));

            init_waitqueue_head(&(kgpudev.reqq));

            spin_lock_init(&(kgpudev.ridlock));
            spin_lock_init(&(kgpudev.gmpool_lock));
            spin_lock_init(&(kgpudev.vm_lock));

            memset(&kgpudev.vm, 0, sizeof(struct _kgpu_vma));

            kgpudev.rid_sequence = 0;

            kgpu_request_cache = kmem_cache_create(
        	"kgpu_request_cache", sizeof(struct kgpu_request), 0,
        	SLAB_HWCACHE_ALIGN, kgpu_request_constructor);
            if (!kgpu_request_cache) {
        	kgpu_log(KGPU_LOG_ERROR, "can't create request cache\n");
        	return -EFAULT;
            }

            kgpu_request_item_cache = kmem_cache_create(
        	"kgpu_request_item_cache", sizeof(struct _kgpu_request_item), 0,
        	SLAB_HWCACHE_ALIGN, kgpu_request_item_constructor);
            if (!kgpu_request_item_cache) {
        	kgpu_log(KGPU_LOG_ERROR, "can't create request item cache\n");
        	kmem_cache_destroy(kgpu_request_cache);
        	return -EFAULT;
            }

            kgpu_sync_call_data_cache = kmem_cache_create(
        	"kgpu_sync_call_data_cache", sizeof(struct _kgpu_sync_call_data), 0,
        	SLAB_HWCACHE_ALIGN, kgpu_sync_call_data_constructor);
            if (!kgpu_sync_call_data_cache) {
        	kgpu_log(KGPU_LOG_ERROR, "can't create sync call data cache\n");
        	kmem_cache_destroy(kgpu_request_cache);
        	kmem_cache_destroy(kgpu_request_item_cache);
        	return -EFAULT;
            }


            /* initialize buffer info */
            memset(&kgpudev.gmpool, 0, sizeof(struct _kgpu_mempool));

            /* dev class */
            kgpudev.cls = class_create(THIS_MODULE, "KGPU_DEV_NAME");
            if (IS_ERR(kgpudev.cls)) {
        	result = PTR_ERR(kgpudev.cls);
        	kgpu_log(KGPU_LOG_ERROR, "can't create dev class for KGPU\n");
        	return result;
            }

            /* alloc dev */
            result = alloc_chrdev_region(&kgpudev.devno, 0, 1, KGPU_DEV_NAME);
            devno = MAJOR(kgpudev.devno);
            kgpudev.devno = MKDEV(devno, 0);

            if (result < 0) {
                kgpu_log(KGPU_LOG_ERROR, "can't get major\n");
            } else {
        	struct device *device;
        	memset(&kgpudev.cdev, 0, sizeof(struct cdev));
        	cdev_init(&kgpudev.cdev, &kgpu_ops);
        	kgpudev.cdev.owner = THIS_MODULE;
        	kgpudev.cdev.ops = &kgpu_ops;
        	result = cdev_add(&kgpudev.cdev, kgpudev.devno, 1);
        	if (result) {
        	    kgpu_log(KGPU_LOG_ERROR, "can't add device %d", result);
        	}

        	/* create /dev/kgpu */
        	device = device_create(kgpudev.cls, NULL, kgpudev.devno, NULL,
        			       KGPU_DEV_NAME);
        	if (IS_ERR(device)) {
        	    kgpu_log(KGPU_LOG_ERROR, "creating device failed\n");
        	    result = PTR_ERR(device);
        	}
            }

            return result;
        }

        static void kgpu_cleanup(void)
        {
            kgpudev.state = KGPU_TERMINATED;

            device_destroy(kgpudev.cls, kgpudev.devno);
            cdev_del(&kgpudev.cdev);
            class_destroy(kgpudev.cls);

            unregister_chrdev_region(kgpudev.devno, 1);
            if (kgpu_request_cache)
        	kmem_cache_destroy(kgpu_request_cache);
            if (kgpu_request_item_cache)
        	kmem_cache_destroy(kgpu_request_item_cache);
            if (kgpu_sync_call_data_cache)
        	kmem_cache_destroy(kgpu_sync_call_data_cache);

            clear_gpu_mempool();
            clean_vm();
        }
    	result = PTR_ERR(kgpudev.cls);
    	kgpu_log(KGPU_LOG_ERROR, "can't create dev class for KGPU\n");
    	return result;
        }

        /* alloc dev */
        result = alloc_chrdev_region(&kgpudev.devno, 0, 1, KGPU_DEV_NAME);
        devno = MAJOR(kgpudev.devno);
        kgpudev.devno = MKDEV(devno, 0);

        if (result < 0) {
            kgpu_log(KGPU_LOG_ERROR, "can't get major\n");
        } else {
    	struct device *device;
    	memset(&kgpudev.cdev, 0, sizeof(struct cdev));
    	cdev_init(&kgpudev.cdev, &kgpu_ops);
    	kgpudev.cdev.owner = THIS_MODULE;
    	kgpudev.cdev.ops = &kgpu_ops;
    	result = cdev_add(&kgpudev.cdev, kgpudev.devno, 1);
    	if (result) {
    	    kgpu_log(KGPU_LOG_ERROR, "can't add device %d", result);
    	}

    	/* create /dev/kgpu */
    	device = device_create(kgpudev.cls, NULL, kgpudev.devno, NULL,
    			       KGPU_DEV_NAME);
    	if (IS_ERR(device)) {
    	    kgpu_log(KGPU_LOG_ERROR, "creating device failed\n");
    	    result = PTR_ERR(device);
    	}
        }

        return result;
    }

    static void kgpu_cleanup(void)
    {
        kgpudev.state = KGPU_TERMINATED;

        device_destroy(kgpudev.cls, kgpudev.devno);
        cdev_del(&kgpudev.cdev);
        class_destroy(kgpudev.cls);

        unregister_chrdev_region(kgpudev.devno, 1);
        if (kgpu_request_cache)
    	kmem_cache_destroy(kgpu_request_cache);
        if (kgpu_request_item_cache)
    	kmem_cache_destroy(kgpu_request_item_cache);
        if (kgpu_sync_call_data_cache)
    	kmem_cache_destroy(kgpu_sync_call_data_cache);

        clear_gpu_mempool();
        clean_vm();
    }
	kmem_cache_destroy(kgpu_request_item_cache);
    if (kgpu_sync_call_data_cache)
	kmem_cache_destroy(kgpu_sync_call_data_cache);

    clear_gpu_mempool();
    clean_vm();
}














static int __init mod_init(void)
{
    //qhgpu_log(QHGPU_LOG_PRINT, "QHGPU loaded\n");
    return qhgpu_init();
}

static void __exit mod_exit(void)
{
	qhgpu_cleanup();
	//qhgpu_log(QHGPU_LOG_PRINT, "QHGPU unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Quadra-H");
MODULE_DESCRIPTION("GPU computing framework for kernel");
