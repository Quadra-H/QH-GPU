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

#include "../../qhgpu/qhgpu.h"



#define BATCH_NR 3
#define MAX_MEM_SZ (1024)
#define MIN_MEM_SZ (128)



// mapped memory address
static char* mmap_addr;
const int ARRAY_SIZE = 1<<20;
const int MAX_INT = 1<<29;

//#define _N (1<<23)
static unsigned int* result;
//static unsigned int* h_keys;
static unsigned int* b;


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


	//h_keys = ( unsigned int * ) kmalloc( ARRAY_SIZE * ( sizeof( unsigned int ) ), GFP_ATOMIC);


	/*if(h_keys==NULL){
		printk("kmalloc fail !! \n");
	}else{*/







		printk("radix_sort_module var init\n");

		buf = qhgpu_vmalloc(sizeof( int ));
		if (!buf) {
			printk("GPU buffer is null.\n");
			return -EFAULT;
		}

		req->in = buf;																			// for hin
		req->out = (void*)((unsigned long)(req->in)+( sizeof( int )));	// for hout
		req->insize = sizeof(buf);
		req->outsize = sizeof((1 * ( sizeof( int ))));
		strcpy(req->service_name, "radix_service");
		req->udatasize = (1 * ( sizeof( int )));
		req->udata = buf;																			// for hdata
		req->callback = test_gpu_callback;
		//////////////////////////////////////////////////////
		// init mmap_addr
		//////////////////////////////////////////////////////
		req->kmmap_addr = qhgpu_mmap_addr_pass();
		req->mmap_size = ARRAY_SIZE;

		//memcpy(req->kmmap_addr, h_keys, req->mmap_size*sizeof(  unsigned int ));



		unsigned int* h_keys = ( unsigned int * )req->kmmap_addr;

		unsigned int i=0;
		unsigned int num;
		for(i = 0; i < ARRAY_SIZE; i++){
			get_random_bytes(&num, sizeof(i));
			h_keys[i] = (num% MAX_INT);
			//h_keys[i] = (num% MAX_INT);
		}



		qhgpu_call_sync(req);


	//}
	return 0;
}

static void __exit mexit(void) {
	printk("test_module exit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
