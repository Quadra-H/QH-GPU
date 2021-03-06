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
#include "cl_helper.h"

#define PAGE_SIZE 512
#define QHGPU_BUF_UNIT_SIZE (1024*50)
#define QHGPU_BUF_NR_FRAMES_PER_UNIT (QHGPU_BUF_UNIT_SIZE/PAGE_SIZE)

static struct kmem_cache *qhgpu_request_cache;
static struct kmem_cache *qhgpu_request_item_cache;
static struct kmem_cache *qhgpu_sync_call_data_cache;


//
static struct qhgpu_module** module_arr;
static char** mmap_addr_arr;
static int* module_active_checker;
static int module_cnt=0;
//


struct _qhgpu_mempool {
	unsigned long uva;
	unsigned long kva;
	struct page **pages;
	u32 npages;
	u32 nunits;
	unsigned long *bitmap;
	u32 *alloc_sz;
};

struct _qhgpu_vma {
	struct vm_area_struct *vma;
	unsigned long start;
	unsigned long end;
	u32 npages;
	u32 *alloc_sz;
	unsigned long *bitmap;
};

struct _qhgpu_dev {
	struct cdev cdev;
	struct class *cls;
	dev_t devno;

	int rid_sequence;
	spinlock_t ridlock;

	struct list_head reqs;
	spinlock_t reqlock;
	wait_queue_head_t reqq;

	struct list_head rtdreqs;
	spinlock_t rtdreqlock;

	struct _qhgpu_mempool gmpool;
	spinlock_t gmpool_lock;

	struct _qhgpu_vma vm;
	spinlock_t vm_lock;


	int state;

	char* mmap_private_data;

	char** kmmap_addr_arr;

};

struct _qhgpu_request_item {
	struct list_head list;
	struct qhgpu_request *r;
};

struct _qhgpu_sync_call_data {
	wait_queue_head_t queue;
	void* oldkdata;
	qhgpu_callback oldcallback;
	int done;
};

static struct _qhgpu_dev qhgpudev;
static atomic_t qhgpu_av = ATOMIC_INIT(1);

int qhgpu_next_request_id(void) {
	int rt = -1;

	spin_lock(&(qhgpudev.ridlock));

	qhgpudev.rid_sequence++;
	if (qhgpudev.rid_sequence < 0)
		qhgpudev.rid_sequence = 0;
	rt = qhgpudev.rid_sequence;

	spin_unlock(&(qhgpudev.ridlock));

	return rt;
}
EXPORT_SYMBOL_GPL( qhgpu_next_request_id);

static void qhgpu_request_item_constructor(void *data) {
	struct _qhgpu_request_item *item = (struct _qhgpu_request_item*) data;

	if (item) {
		memset(item, 0, sizeof(struct _qhgpu_request_item));
		INIT_LIST_HEAD(&item->list);
		item->r = NULL;
	}
}

static void qhgpu_sync_call_data_constructor(void *data) {
	struct _qhgpu_sync_call_data *d = (struct _qhgpu_sync_call_data*) data;

	if (d) {
		memset(d, 0, sizeof(struct _qhgpu_sync_call_data));
	}
}

static void qhgpu_request_constructor(void* data) {
	struct qhgpu_request *req = (struct qhgpu_request*) data;

	if (req) {
		memset(req, 0, sizeof(struct qhgpu_request));
		req->id = qhgpu_next_request_id();
		req->service_name[0] = 0;
	}
}

struct page* qhgpu_v2page(unsigned long v) {
	struct page *p = NULL;
	pgd_t *pgd = pgd_offset(current->mm, v);

	if (!pgd_none(*pgd)) {
		pud_t *pud = pud_offset(pgd, v);
		if (!pud_none(*pud)) {
			pmd_t *pmd = pmd_offset(pud, v);
			if (!pmd_none(*pmd)) {
				pte_t *pte;
				pte = pte_offset_map(pmd, v);
				if (pte_present(*pte))
					p = pte_page(*pte);

				pte_unmap(pte);
			}
		}
	}
	if (!p)
		printk("bad address 0x%lX\n", v);

	return p;
}

int qhgpu_open(struct inode *inode, struct file *filp) {
	printk("open called\n");

	if (!atomic_dec_and_test(&qhgpu_av)) {
		atomic_inc(&qhgpu_av);
		return -1;
	}

	filp->private_data = &qhgpudev;
	return 0;
}

/*
 * find request by id in the rtdreqs
 * offlist = 1: remove the request from the list
 * offlist = 0: keep the request in the list
 */
static struct _qhgpu_request_item* find_request(int id, int offlist) {
	struct _qhgpu_request_item *pos, *n;

	spin_lock(&(qhgpudev.rtdreqlock));

	list_for_each_entry_safe(pos, n, &(qhgpudev.rtdreqs), list)
	{
		if (pos->r->id == id) {
			if (offlist)
				list_del(&pos->list);
			spin_unlock(&(qhgpudev.rtdreqlock));
			return pos;
		}
	}
	spin_unlock(&(qhgpudev.rtdreqlock));
	return NULL;
}

ssize_t qhgpu_write(struct file *filp, const char __user *buf, size_t count, loff_t *fpos) {
	struct qhgpu_ku_response kuresp;
	struct _qhgpu_request_item *item;
	ssize_t ret = 0;
	size_t realcount;

	//printk("qhgpu_write called\n");

	if (count < sizeof(struct qhgpu_ku_response))
	ret = -EINVAL; /* Too small. */
	else
	{
		realcount = sizeof(struct qhgpu_ku_response);

		memcpy/*copy_from_user*/(&kuresp, buf, realcount);

		item = find_request(kuresp.id, 1);
		if (!item)
		{
			printk("no request found !! \n");
			ret = -EFAULT; /* no request found */
		} else {

			//printk("request found !! %d \n",item->r->errcode);

			item->r->errcode = kuresp.errcode;
			/*if (unlikely(kuresp.errcode != 0)) {
			 switch(kuresp.errcode) {
			 case QHGPU_NO_RESPONSE:
			 printk("userspace helper doesn't give any response\n");
			 break;
			 case QHGPU_NO_SERVICE:
			 printk("no such service %s\n",item->r->service_name);
			 break;

			 case QHGPU_TERMINATED:
			 printk("request is terminated\n");
			 break;

			 default:
			 printk("unknown error with code %d\n",kuresp.id);
			 break;
			 }
			 }*/
			item->r->callback(item->r);
			//printk("callback done !! \n");
			ret = count;/*realcount;*/
			*fpos += ret;
			kmem_cache_free(qhgpu_request_item_cache, item);
		}
	}
	//printk("qhgpu_write ret !! %d\n",ret);
	return ret;
}

static void fill_ku_request(struct qhgpu_ku_request *kureq, struct qhgpu_request *req) {
	kureq->id = req->id;
	memcpy(kureq->service_name, req->service_name, QHGPU_SERVICE_NAME_SIZE);


	//kureq->mmap_addr = qhgpudev.mmap_private_data;
	kureq->kmmap_addr = req->kmmap_addr;
	kureq->mmap_size = req->mmap_size;
	kureq->batch_size = req->batch_size;




	//////////////////[?]
	if (ADDR_WITHIN(req->in, qhgpudev.gmpool.kva,
			qhgpudev.gmpool.npages<<PAGE_SHIFT)) {
		kureq->in = (void*) ADDR_REBASE(qhgpudev.gmpool.uva,
				qhgpudev.gmpool.kva,
				req->in);
	} else {
		kureq->in = req->in;
	}


	/*int *nm = (int*)kureq->in;
	nm[0]=*((int*)req->in);
	printk("========================== \n");
	printk("num: %d \n",nm[0]);
	printk("========================== \n");*/

	if (ADDR_WITHIN(req->out, qhgpudev.gmpool.kva,
			qhgpudev.gmpool.npages<<PAGE_SHIFT)) {
		kureq->out = (void*) ADDR_REBASE(qhgpudev.gmpool.uva,
				qhgpudev.gmpool.kva,
				req->out);
	} else {
		kureq->out = req->out;
	}

	if (ADDR_WITHIN(req->udata, qhgpudev.gmpool.kva,
			qhgpudev.gmpool.npages<<PAGE_SHIFT)) {
		kureq->data = (void*) ADDR_REBASE(qhgpudev.gmpool.uva,
				qhgpudev.gmpool.kva,
				req->udata);
	} else {
		kureq->data = req->udata;
	}
	//////////////////
	/*nm = (int*)kureq->data;
	nm[0]=*((int*)req->udata);
	printk("========================== \n");
	printk("data: %d \n",nm[0]);
	printk("========================== \n");
	 */



	kureq->insize = req->insize;
	kureq->outsize = req->outsize;
	kureq->datasize = req->udatasize;


	//printk("{%p} fill_ku_request \n",req->mmap_addr);
}

static int set_gpu_mempool(char __user *buf) {
	struct qhgpu_gpu_mem_info gb;
	struct _qhgpu_mempool *gmp = &qhgpudev.gmpool;
	int i;
	int err=0;

	spin_lock(&(qhgpudev.gmpool_lock));

	// user to kernel copy_from_user(to, from, number of bytes to copy);
	copy_from_user(&gb, buf, sizeof(struct qhgpu_gpu_mem_info));

	// set up pages mem
	gmp->uva = (unsigned long)(gb.uva);
	gmp->npages = gb.size/PAGE_SIZE;
	if (!gmp->pages) {
		gmp->pages = kmalloc(sizeof(struct page*)*gmp->npages, GFP_KERNEL);	// kernel memory alloc
		if (!gmp->pages) {
			printk("run out of memory for gmp pages\n");
			err = -ENOMEM;
			goto unlock_and_out;
		}
	}

	for (i=0; i<gmp->npages; i++)
	gmp->pages[i]= qhgpu_v2page(
			(unsigned long)(gb.uva) + i*PAGE_SIZE
	);



	printk("gmp->npages : %d , QHGPU_BUF_NR_FRAMES_PER_UNIT: %d PAGE_SIE: %d\n",gmp->npages,QHGPU_BUF_NR_FRAMES_PER_UNIT,PAGE_SIZE);
	/* set up bitmap */
	gmp->nunits = gmp->npages/QHGPU_BUF_NR_FRAMES_PER_UNIT;
	if (!gmp->bitmap) {
		gmp->bitmap = kmalloc(
				BITS_TO_LONGS(gmp->nunits)*sizeof(long), GFP_KERNEL);
		if (!gmp->bitmap) {
			//qhgpu_log(KGPU_LOG_ERROR, "run out of memory for gmp bitmap\n");
			err = -ENOMEM;
			goto unlock_and_out;
		}
	}
	bitmap_zero(gmp->bitmap, gmp->nunits);

	if (!gmp->alloc_sz) {
		gmp->alloc_sz = kmalloc(
				gmp->nunits*sizeof(u32), GFP_KERNEL);
		if (!gmp->alloc_sz) {
			printk("run out of memory for gmp alloc_sz\n");
			err = -ENOMEM;
			goto unlock_and_out;
		}
	}
	memset(gmp->alloc_sz, 0, gmp->nunits);

	/* set up kernel remapping */
	gmp->kva = (unsigned long)vmap(
			gmp->pages, gmp->npages, GFP_KERNEL, PAGE_KERNEL);
	if (!gmp->kva) {
		printk("map pages into kernel failed\n");
		err = -EFAULT;
		goto unlock_and_out;
	}
	unlock_and_out:
	spin_unlock(&(qhgpudev.gmpool_lock));

	return err;
}
//static long device_ioctl(struct file *file, unsigned int cmd_in, unsigned long arg)

ssize_t qhgpu_read(struct file *filp, char __user *buf, size_t c, loff_t *fpos) {
	ssize_t ret = 0;
	struct list_head *r;
	struct _qhgpu_request_item *item;

	//printk("qhgpu_read start !! \n");

	////////////////////////////////////////////////////////////////////////
	// call --> qhgpudev.reqs --> connector's qhgpu_ku_request
	// get request item from qhgpudev.reqs
	////////////////////////////////////////////////////////////////////////
	spin_lock(&(qhgpudev.reqlock));

	while (list_empty(&(qhgpudev.reqs))) {		// if requests are empty

		spin_unlock(&(qhgpudev.reqlock));

		if (filp->f_flags & O_NONBLOCK)
		return -EAGAIN;

		//  The function will return -ERESTARTSYS if it was interrupted by a signal and
		//	 0 if condition evaluated to true.
		if (wait_event_interruptible(
						qhgpudev.reqq, (!list_empty(&(qhgpudev.reqs)))))
		return -ERESTARTSYS;

		spin_lock(&(qhgpudev.reqlock));
	}

	r = qhgpudev.reqs.next;
	list_del(r);
	item = list_entry(r, struct _qhgpu_request_item, list);	/// get request item

	//printk("qhgpu_read item id: %d  \n",item->r->id);
	if (item) {
		struct qhgpu_ku_request kureq;
		fill_ku_request(&kureq, item->r);

		memcpy(buf, &kureq, sizeof(struct qhgpu_ku_request));// 	void *memcpy(void *dest, const void *sour, size_t n);
		ret = c;
	}

	spin_unlock(&(qhgpudev.reqlock));
	////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////
	// remove request from reqs and add to rtdreqs
	// get request item from qhgpudev.reqs
	////////////////////////////////////////////////////////////////////////
	if (ret > 0 && item) {
		spin_lock(&(qhgpudev.rtdreqlock));
		INIT_LIST_HEAD(&item->list);

		//printk("add item to qhgpudev.rtdreqs !! \n");
		list_add_tail(&item->list, &(qhgpudev.rtdreqs));	///// rtdreqs [?]
		spin_unlock(&(qhgpudev.rtdreqlock));
	}
	////////////////////////////////////////////////////////////////////////

	*fpos += ret;/// move file pointer sizeof(struct qhgpu_ku_request)

	return ret;
}

static long qhgpu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	int err = 0;

	if (_IOC_TYPE(cmd) != QHGPU_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > QHGPU_IOC_MAXNR)
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {

	case QHGPU_IOC_SET_GPU_BUFS:
		printk("qhgpu_ioctl QHGPU_IOC_SET_GPU_BUFS. \n",arg);
		err = set_gpu_mempool((char*) arg);
		break;

	case QHGPU_IOC_SET_MMAP:
		printk("qhgpu_ioctl QHGPU_IOC_SET_MMAP.  %ld\n",arg);


		module_cnt = arg;
		////// multi module setting
		module_active_checker=(int*)vmalloc(sizeof(int*)*arg);
		qhgpudev.kmmap_addr_arr=(char**)vmalloc(sizeof(char*)*arg);
		mmap_addr_arr = (char**)vmalloc(sizeof(char*)*arg);
		module_arr=(char**)vmalloc(sizeof(struct qhgpu_module*)*arg);

		int i=0;
		for(i=0;i<arg;i++){

			mmap_addr_arr[i] = (char *)__get_free_pages(GFP_KERNEL, 10);
			if( mmap_addr_arr[i] == NULL ) {
				printk("qhgpu_mmap alloc free pages error %d\n",i);
				return -1;
			}
			printk("[qhpgpu module]mmap_addr_arr alloc. [%p]\n", mmap_addr_arr[i]);
		}
		////////////////////////////////////


		break;

	case QHGPU_IOC_BATCH_H2D:
		printk("qhgpu_ioctl QHGPU_IOC_BATCH_H2D.  %ld\n",arg);
		module_arr[arg]->batch_data_loader();

		break;
	case QHGPU_IOC_BATCH_D2H:
		printk("qhgpu_ioctl QHGPU_IOC_BATCH_D2H.  %ld\n",arg);
		module_arr[arg]->batch_data_result();


		break;
	case QHGPU_IOC_SET_STOP:
		printk("qhgpu_ioctl QHGPU_IOC_SET_STOP. \n");
		//err = terminate_all_requests();
		break;


	default:	//
		err = -ENOTTY;
		break;
	}

	return err;
}

/*
 * vm_ops
 * for KU communication by mmap
 */

//can track mmap by open or close
/*void vm_open(struct vm_area_struct *vma) {
	printk("vm_open\n");
	//printk(KERN_NOTICE "Simple VMA open, virt %lx, phys %lx\n", vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

void vm_close(struct vm_area_struct *vma) {
	printk("vm_close\n");
}*/

/* fault is called the first time a memory area is accessed which is not in memory,
 * it does the actual mapping between kernel and user space memory
 */
static int vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf) {
	struct page *page;
	char* mmap_buf = (char*) vma->vm_private_data;

	//printk("vm_fault called info=0x%8X ,", (unsigned int) mmap_buf);
	//printk("vm_fault vma info. {%p, %p} , ", vma, vma->vm_private_data);


	//vmf->page = my_page_at_index(vmf->pgoff);
	//get_page(vmf->page);


	// the data is in vma->vm_private_data
	if (!mmap_buf) {
		printk("mmap_fault return VM_FAULT_OOM\n");
		return VM_FAULT_OOM;
	}

	page = virt_to_page(mmap_buf+ ( vmf->pgoff << PAGE_SHIFT));

	if (!page) {
		printk("vm_fault return VM_FAULT_SIGBUS\n");
		return VM_FAULT_SIGBUS;
	}

	get_page(page);
	vmf->page = page;

	//printk("vm_fault return 0");
	return 0;
}

struct vm_operations_struct qhgpu_vm_ops = {
		//.open = vm_open,
		//.close = vm_close,
		.fault = vm_fault,
};
// END vm_ops ////////////////////////////////


//
static int mmap_count = 0;
//

static int qhgpu_mmap(struct file *filp, struct vm_area_struct *vma) {
	//printk("qhgpu_mmap. filep data. {%p, %p}\n", filp, filp->private_data);
	//printk("qhgpu_mmap vma info. {%p, %p}\n", vma, vma->vm_private_data);

	vma->vm_ops = &qhgpu_vm_ops;
	vma->vm_flags |= VM_IO;
	/* assign the file private data to the vm private data */

	//b
	/*filp->private_data = (char *)__get_free_pages(GFP_KERNEL, 10);
	if( filp->private_data == NULL ) {
		printk("qhgpu_mmap alloc free pages error\n");
		return -1;
	}*/


	//b
	//qhgpudev.mmap_private_data = filp->private_data;

	printk("[qhgpu_module]mmap %d\n", mmap_count);

	//
	filp->private_data = mmap_addr_arr[mmap_count];
	qhgpudev.kmmap_addr_arr[mmap_count] = filp->private_data;
	mmap_count++;
	//

	vma->vm_private_data = filp->private_data;
	//vm_open(vma);

	return 0;
}


static unsigned int qhgpu_poll(struct file *filp, poll_table *wait) {
	unsigned int mask = 0;

	spin_lock(&(qhgpudev.reqlock));
	poll_wait(filp, &(qhgpudev.reqq), wait);// wait_queue_head_t type reqq wait

	if (!list_empty(&(qhgpudev.reqs)))			// check request list
		mask |= POLLIN | POLLRDNORM;			// queue readable
		mask |= POLLOUT | POLLWRNORM;				// queue writable
	spin_unlock(&(qhgpudev.reqlock));

	return mask;
}

static struct file_operations qhgpu_ops = {
		.owner = THIS_MODULE,
		.open = qhgpu_open,
		.read = qhgpu_read,
		.unlocked_ioctl = qhgpu_ioctl,
		.write = qhgpu_write,
		.poll = qhgpu_poll,
		.mmap = qhgpu_mmap
		//.llseek = no_llseek,
};

static int qhgpu_init(void) {
	int result = 0;
	int devno;

	qhgpudev.state = QHGPU_OK;

	INIT_LIST_HEAD(&(qhgpudev.reqs));
	INIT_LIST_HEAD(&(qhgpudev.rtdreqs));

	spin_lock_init(&(qhgpudev.reqlock));
	spin_lock_init(&(qhgpudev.rtdreqlock));

	init_waitqueue_head(&(qhgpudev.reqq));

	spin_lock_init(&(qhgpudev.ridlock));
	spin_lock_init(&(qhgpudev.gmpool_lock));
	spin_lock_init(&(qhgpudev.vm_lock));

	memset(&qhgpudev.vm, 0, sizeof(struct _qhgpu_vma));

	qhgpudev.rid_sequence = 0;

	qhgpu_request_cache = kmem_cache_create("qhgpu_request_cache",
			sizeof(struct qhgpu_request), 0, SLAB_HWCACHE_ALIGN,
			qhgpu_request_constructor);
	if (!qhgpu_request_cache) {
		printk("can't create request cache\n");
		return -EFAULT;
	}
	qhgpu_request_item_cache = kmem_cache_create("qhgpu_request_item_cache",
			sizeof(struct _qhgpu_request_item), 0, SLAB_HWCACHE_ALIGN,
			qhgpu_request_item_constructor);
	if (!qhgpu_request_item_cache) {
		printk("can't create request item cache\n");
		kmem_cache_destroy(qhgpu_request_cache);
		return -EFAULT;
	}

	qhgpu_sync_call_data_cache = kmem_cache_create("qhgpu_sync_call_data_cache",
			sizeof(struct _qhgpu_sync_call_data), 0, SLAB_HWCACHE_ALIGN,
			qhgpu_sync_call_data_constructor);
	if (!qhgpu_sync_call_data_cache) {
		printk("can't create sync call data cache\n");
		kmem_cache_destroy(qhgpu_request_cache);
		kmem_cache_destroy(qhgpu_request_item_cache);
		return -EFAULT;
	}

	/* initialize buffer info */
	memset(&qhgpudev.gmpool, 0, sizeof(struct _qhgpu_mempool));

	/* dev class */
	qhgpudev.cls = class_create(THIS_MODULE, "QHGPU_DEV_NAME");
	if (IS_ERR(qhgpudev.cls)) {
		result = PTR_ERR(qhgpudev.cls);
		printk("can't create dev class for qhgpu\n");
		return result;
	}

	// initialize buffer info
	memset(&qhgpudev.gmpool, 0, sizeof(struct _qhgpu_mempool));

	/*
	// dev class
	qhgpudev.cls = class_create(THIS_MODULE, "QHGPU_DEV_NAME");
	if (IS_ERR(qhgpudev.cls)) {
	result = PTR_ERR(qhgpudev.cls);
	printk("can't create dev class for KGPU\n");
	return result;
	}
	*/

	// alloc dev
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

		// create /dev/qhgpu
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

	//b
	//free mmap buffer
	//free_pages(qhgpudev.mmap_private_data, 10);

	//todo : free_pages

	return 1;
}


void qhgpu_free_request(struct qhgpu_request* req) {
	kmem_cache_free(qhgpu_request_cache, req);
}
EXPORT_SYMBOL_GPL( qhgpu_free_request);

struct qhgpu_request* qhgpu_alloc_request(int data_size, char *service_name) {
	struct qhgpu_request *req = kmem_cache_alloc(qhgpu_request_cache,
			GFP_KERNEL);

	strcpy(req->service_name, service_name);
	req->id = qhgpu_mmap_id();

	/*int *id_buf = (int*)qhgpu_vmalloc(sizeof( int ));
	printk("req->id %d  \n", req->id, id_buf[0]);
	req->udata = id_buf;

	int *batch_cnt = (int*)qhgpu_vmalloc(sizeof( int ));				////batch count
	batch_cnt[0]=(data_size%_MAX_BATCH_SIZE==0) ? data_size/_MAX_BATCH_SIZE:(data_size/_MAX_BATCH_SIZE)+1;
	req->in = batch_cnt;*/

	/*int *batch_out_cnt = (int*)qhgpu_vmalloc(sizeof( int ));				////batch count
	batch_out_cnt[0]=(data_size%_MAX_BATCH_SIZE==0) ? data_size/_MAX_BATCH_SIZE:(data_size/_MAX_BATCH_SIZE)+1;
	req->out = batch_out_cnt;
	*/

	//printk("in,out ,batch_cnt: %d \n", ((int*)(req->in))[0]);





	req->kmmap_addr = qhgpu_mmap_addr_pass(req->id);
	req->mmap_size = data_size;
	req->batch_size = (data_size%_MAX_BATCH_SIZE==0) ? data_size/_MAX_BATCH_SIZE:(data_size/_MAX_BATCH_SIZE)+1;

	return req;
}
EXPORT_SYMBOL_GPL( qhgpu_alloc_request);

/*
 * Callback for sync GPU call.
 */
static int sync_callback(struct qhgpu_request *req) {
	struct _qhgpu_sync_call_data *data =
			(struct _qhgpu_sync_call_data*) req->kdata;

	//printk("sync_callback !!!!\n");

	data->done = 1;
	wake_up_interruptible(&data->queue);
	return 0;
}

/*
 * Sync GPU call
 */
int qhgpu_call_sync(struct qhgpu_request *req) {
	struct _qhgpu_sync_call_data *data;
	struct _qhgpu_request_item *item;

	//printk("qhgpu_call_sync START~!!!\n");

	if (unlikely(qhgpudev.state == QHGPU_TERMINATED)) {
		printk("qhgpu is terminated, no request accepted any more\n");
		return QHGPU_TERMINATED;
	}

	data = kmem_cache_alloc(qhgpu_sync_call_data_cache, GFP_KERNEL);
	if (!data) {
		printk("qhgpu_call_sync alloc mem failed\n");
		return -ENOMEM;
	}
	item = kmem_cache_alloc(qhgpu_request_item_cache, GFP_KERNEL);

	if (!item) {
		printk("out of memory for qhgpu request\n");
		return -ENOMEM;
	}
	item->r = req;

	data->oldkdata = req->kdata;
	data->oldcallback = req->callback;
	data->done = 0;
	init_waitqueue_head(&data->queue);


	req->kdata = data;
	req->callback = sync_callback;
	module_arr[req->id] = req->module;



	spin_lock(&(qhgpudev.reqlock));

	INIT_LIST_HEAD(&item->list);

	list_add_tail(&item->list, &(qhgpudev.reqs));

	wake_up_interruptible(&(qhgpudev.reqq));

	spin_unlock(&(qhgpudev.reqlock));

	wait_event_interruptible(data->queue, (data->done == 1));

	req->kdata = data->oldkdata;
	req->callback = data->oldcallback;
	kmem_cache_free(qhgpu_sync_call_data_cache, data);


	//printk("qhgpu_call_sync DONE~!!!\n");

	///////////////////////////////////////////////////
	qhgpu_return_mmap(req->id);
	///////////////////////////////////////////////////
	return 0;
}
EXPORT_SYMBOL_GPL( qhgpu_call_sync);




//
//int qhgpu_call_n_async(struct qhgpu_request **req, struct completion* cs, int n){
//
//	int i=0;
//	for(int i=0;i<n;i++){
//		qhgpu_call_async(req[i]);
//	}
//
//	for (i=0; i<BATCH_NR; i++)
//		wait_for_completion(cs+i);
//
//}

/*
 * Async GPU call.
 */
int qhgpu_call_async(struct qhgpu_request *req)
{

	struct _qhgpu_request_item *item;
	if (unlikely(qhgpudev.state == QHGPU_TERMINATED)) {
		printk("qhgpu is terminated, no request accepted any more\n");
		return QHGPU_TERMINATED;
	}

	item = kmem_cache_alloc(qhgpu_request_item_cache, GFP_KERNEL);

	if (!item) {
		printk("out of memory for qhgpu request\n");
		return -ENOMEM;
	}
	item->r = req;

	spin_lock(&(qhgpudev.reqlock));

	INIT_LIST_HEAD(&item->list);
	list_add_tail(&item->list, &(qhgpudev.reqs));
	//printk("wake up~~~~~~~~~~~~~~~~~~~~~~~!!!!!\n");
	wake_up_interruptible(&(qhgpudev.reqq));

	spin_unlock(&(qhgpudev.reqlock));
	return 0;
}
EXPORT_SYMBOL_GPL(qhgpu_call_async);








void* qhgpu_vmalloc(unsigned long nbytes)
{
	unsigned int req_nunits = DIV_ROUND_UP(nbytes, QHGPU_BUF_UNIT_SIZE);
	void *p = NULL;
	unsigned long idx;

	spin_lock(&qhgpudev.gmpool_lock);



	/*
	bitmap_find_next_zero_area - find a contiguous aligned zero area
	@map: The address to base the search on
	@size: The bitmap size in bits
	@start: The bitnumber to start searching at
	@nr: The number of zeroed bits we're looking for
	@align_mask: Alignment mask for zero area

	* The @align_mask should be one less than a power of 2; the effect is that
	* the bit offset of all zero areas this function finds is multiples of that
	* power of 2. A @align_mask of 0 means no alignment is required.
	*/
	idx = bitmap_find_next_zero_area(
			qhgpudev.gmpool.bitmap, // map
			qhgpudev.gmpool.nunits, // size
			0, 							// start
			req_nunits, 				// nr
			0);							// align_mask


	printk("idx: %d, gmpool.nunits: %d ,%d,%d \n",idx,qhgpudev.gmpool.nunits,nbytes,req_nunits);


	if (idx < qhgpudev.gmpool.nunits) {
		printk("bitmap_set start \n");
		bitmap_set(qhgpudev.gmpool.bitmap, idx, req_nunits);
		printk("bitmap_set end \n");
    	p = (void*)((unsigned long)(qhgpudev.gmpool.kva)
    			+ idx*QHGPU_BUF_UNIT_SIZE);
    	printk("qhgpudev.gmpool.kva p: %p \n",p);
    	qhgpudev.gmpool.alloc_sz[idx] = req_nunits;
	} else {
		printk("out of GPU memory for malloc %lu\n",nbytes);
	}
	spin_unlock(&qhgpudev.gmpool_lock);
	return p;
}
EXPORT_SYMBOL_GPL( qhgpu_vmalloc);

void qhgpu_vfree(void *p) {
	unsigned long idx = ((unsigned long) (p)
			- (unsigned long) (qhgpudev.gmpool.kva)) / QHGPU_BUF_UNIT_SIZE;
	unsigned int nunits;

	if (idx < 0 || idx >= qhgpudev.gmpool.nunits) {
		printk("incorrect GPU memory pointer 0x%p to free\n", p);
		return;
	}

	nunits = qhgpudev.gmpool.alloc_sz[idx];
	if (nunits == 0) {
		return;
	}
	if (nunits > (qhgpudev.gmpool.nunits - idx)) {
		printk(
				"incorrect GPU memory allocation info: allocated %u units at unit index %lu\n",
				nunits, idx);
		return;
	}
	spin_lock(&qhgpudev.gmpool_lock);
	bitmap_clear(qhgpudev.gmpool.bitmap, idx, nunits);
	qhgpudev.gmpool.alloc_sz[idx] = 0;
	spin_unlock(&qhgpudev.gmpool_lock);
}
EXPORT_SYMBOL_GPL( qhgpu_vfree);




char* qhgpu_mmap_addr_pass(int i) {
	printk("[qhgpu_module]mmap_addr_pass %d [%p]\n", i, qhgpudev.kmmap_addr_arr[i]);

	//return qhgpudev.mmap_private_data;
	return qhgpudev.kmmap_addr_arr[i];
}
EXPORT_SYMBOL_GPL(qhgpu_mmap_addr_pass);

int qhgpu_mmap_id() {
	//return qhgpudev.mmap_private_data;

	int i=0;
	for(i=0;i<module_cnt;i++){
		if(module_active_checker[i]==0){
			module_active_checker[i]=1;
			return i;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(qhgpu_mmap_id);

void qhgpu_return_mmap(int module_id) {

	//printk("return mmap !! \n");
	module_active_checker[module_id]=0;

}
EXPORT_SYMBOL_GPL(qhgpu_return_mmap);



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
