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

// mapped memory address
static char* mmap_addr;
//const int ARRAY_SIZE = 1<<20;
//const int MAX_INT = 1<<24;

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

static int __init minit(void)
{
	printk("test_module init\n");
	//struct completion cs[BATCH_NR];

	int err=0;
	size_t nbytes;
	unsigned int cur;

	struct qhgpu_request *req;
	char *buf;

	req = qhgpu_alloc_request();
	if (!req) {
		printk("request null\n");
		return 0;
	}

	strcpy(req->service_name, "libsrv_bfs");
	req->callback = test_gpu_callback;

	req->kmmap_addr = qhgpu_mmap_addr_pass();
	//req->mmap_size = _N;


	qhgpu_call_sync(req);

	printk("bfs module success\n");

	return 0;
}

static void __exit mexit(void) {
	printk("test_module exit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
