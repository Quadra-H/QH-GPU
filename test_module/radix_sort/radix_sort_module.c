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
#include "./lib/cl_radix_sort_param.h"



#define BATCH_NR 3
#define MAX_MEM_SZ (1024)
#define MIN_MEM_SZ (128)



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



int pow(int k, int n){

	int i=0,ret = 1;
	for(i=0;i<n;i++){
		ret*=k;
	}
	return ret;
}

// data : 정수배열
// size : data의 정수들의 개수
// p : 숫자위치의 최대개수 (789라면 3자리숫자이므로 3)
// k : 기수 (10진법을 사용할 것이므로 10)

unsigned int *counts, // 특정자리에서 숫자들의 카운트
      *temp; // 정렬된 배열을 담을 임시장소

void rxSort(int *data, int size, int p, int k) {

  int index, pval, i, j, n;

  // 메모리 할당


  for (n=0; n<p; n++) { // 1의 자리, 10의자리, 100의 자리 순으로 진행
    for (i=0; i<k; i++)
      counts[i] = 0; // 초기화

    // 위치값 계산.
    // n:0 => 1,  1 => 10, 2 => 100
    pval = pow(k, n);


    // 각 숫자의 발생횟수를 센다.
    for (j=0; j<size; j++) {
      // 253이라는 숫자라면
      // n:0 => 3,  1 => 5, 2 => 2
      index = (int)(data[j] / pval) % k;
      counts[index] = counts[index] + 1;
    }



    // 카운트 누적합을 구한다. 계수정렬을 위해서.
    for (i=1; i<k; i++) {
      counts[i] = counts[i] + counts[i-1];
    }



    // 카운트를 사용해 각 항목의 위치를 결정한다.
    // 계수정렬 방식
    for (j=size-1; j>=0; j--) { // 뒤에서 부터 시작
      index = (int)(data[j] / pval) % k;
      temp[counts[index] -1] = data[j];
      counts[index] = counts[index] - 1; // 해당 숫자카운트를 1 감소
    }



    // 임시데이타의 카피
    memcpy(data, temp, size * sizeof(int));
  }
}







static int __init minit(void) {
	printk("test_module init\n");
	struct completion cs[BATCH_NR];

	int err=0;

	size_t nbytes;
	unsigned int cur;
	unsigned long sz;
	struct timeval t0, t1;
	long tt;


	struct qhgpu_request *req;
	char *buf;

	//// alloc request
	req = qhgpu_alloc_request();
	if (!req) {
		printk("request null\n");
		return 0;
	}

	/*
	//// qhgpu dev vmalloc(pinned memory) for request alloc
	buf = qhgpu_vmalloc(sizeof( int ));
	if (!buf) {
		printk("GPU buffer is null.\n");
		return -EFAULT;
	}
	req->in = buf;																			// for hin
	req->out = (void*)((unsigned long)(req->in)+( sizeof( int )));	// for hout
	req->insize = sizeof(buf);
	req->outsize = sizeof((1 * ( sizeof( int ))));
	req->udatasize = (1 * ( sizeof( int )));
	req->udata = buf;																			// for hdata
	*/


	strcpy(req->service_name, "radix_service");
	req->callback = test_gpu_callback;
	//////////////////////////////////////////////////////
	// init mmap_addr
	//////////////////////////////////////////////////////
	req->kmmap_addr = qhgpu_mmap_addr_pass();
	req->mmap_size = _N;



	unsigned int* h_keys = ( unsigned int * )req->kmmap_addr;
	unsigned int i=0;
	unsigned int num;
	for(i = 0; i < _N; i++){
		get_random_bytes(&num, sizeof(i));
		h_keys[i] = (num% _MAXINT);
		//h_keys[i]=i;

	}

	printk("\n\nbefore sort");
	printk("========================\n");
	for(i = 0; i < 50; i++){
		printk("%u ,",h_keys[i]);
	}
	printk("========================\n");
	///////////////////////////////////////////////////


	do_gettimeofday(&t0);
	///////////////////////////////////////////////////
	qhgpu_call_sync(req);
	///////////////////////////////////////////////////
	do_gettimeofday(&t1);

	tt = 1000000*(t1.tv_sec-t0.tv_sec) +
			((long)(t1.tv_usec) - (long)(t0.tv_usec));

	printk("SYNC  SIZE: %10lu B, TIME: %10lu MS, OPS: %8lu, BW: %8lu MB/S\n",
			sz, tt, 1000000/tt, sz/tt);




	/////sync call done/////////////////////////
	req->kmmap_addr = qhgpu_mmap_addr_pass();
	h_keys = ( unsigned int * )req->kmmap_addr;

	printk("\n\n");
	printk("========================\n");
	for(i = 0; i < 50; i++){
		printk("%u ,",h_keys[i]);
	}
	printk("========================\n");
	///////////////////////////////////////////////////









	/* we want DMA-capable memory,
	 * and we can sleep if needed */
	//unsigned int cpu_keys[ARRAY_SIZE];

	if((counts = (unsigned int *)__get_free_pages(GFP_KERNEL, 10))==NULL)return;
	if((temp = (unsigned int *)__get_free_pages(GFP_KERNEL, 10))==NULL)return;


	//if ( (counts = (unsigned int *)vmalloc(_N * ( sizeof( int )), GFP_DMA | GFP_KERNEL)) == NULL )
	/*if ( (counts = (unsigned int *)vmalloc(_N * ( sizeof( int ))) == NULL ))
		return;
	if ( (temp = (unsigned int *)vmalloc(_N * ( sizeof( int ))) == NULL ))
		return;

		*/

	unsigned int* cpu_keys = (unsigned int*)__get_free_pages(GFP_KERNEL, 10);
	if (!cpu_keys){
		printk("malloc fail !!! \n");
	}else{

		printk("malloc success !!! \n");


		for(i = 0; i < _N; i++){
			get_random_bytes(&num, sizeof(i));
			cpu_keys[i] = (num% _MAXINT);
		}


		do_gettimeofday(&t0);
		rxSort(cpu_keys, _N, 29, 2);
		do_gettimeofday(&t1);

		tt = 1000000*(t1.tv_sec-t0.tv_sec) +
				((long)(t1.tv_usec) - (long)(t0.tv_usec));

		printk("SYNC  SIZE: %10lu B, TIME: %10lu MS, OPS: %8lu, BW: %8lu MB/S\n",
				sz, tt, 1000000/tt, sz/tt);


		printk("\n\n");
		printk("========================\n");
		for(i = 0; i < 50; i++){
			printk("%d ,",cpu_keys[i]);
		}
		printk("========================\n");
		///////////////////////////////////////////////////
	}







	return 0;
}

static void __exit mexit(void) {
	printk("test_module exit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
