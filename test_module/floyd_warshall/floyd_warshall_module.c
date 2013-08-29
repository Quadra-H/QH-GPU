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


//#define INT_MAX 9999999
#define INT_MAX 2147483647;


/********************************
 * generate non directional adjacency matrix
 * generate almost (rate/100)% nonzero edge
 ********************************/
void kgenerate_adjacency_matrix(unsigned int* mat, const unsigned int mat_size, unsigned int rate) {
	unsigned int i, j, temp;

	//generate srand
	srand((unsigned)time(NULL)+(unsigned)getpid());

	for(i = 0 ; i < mat_size ; i++ ) {
		mat[i*mat_size + i] = 0;
		for(j = i+1 ; j < mat_size ; j++ ) {
			temp = rand() % 100 + 1;
			if( temp > rate ) {
				temp = INT_MAX;
			}
			mat[j*mat_size + i] = mat[i*mat_size + j] = temp;
		}
	}
}


/********************************
 * print adjacency matrix
 ********************************/
void kprint_adjacency_matrix(unsigned int* adj_mat, const unsigned int MAT_SIZE) {
	int i, j;

	for(i = 0 ; i < MAT_SIZE ; i++ ) {
		for(j = 0 ; j < MAT_SIZE ; j++ ) {
			printk("%8u", adj_mat[i*MAT_SIZE + j]);
		}
		printk("\n");
	}
	printk("\n");
}


/********************************
 * Floyd-Warshall by cpu
 * whitout parallel processing
 ********************************/
int kfloyd_warshall_cpu(unsigned int* adj_mat, const unsigned int MAT_SIZE) {
	int i, j, k;

	for ( k = 0 ; k < MAT_SIZE ; k++ ) {
		for ( i = 0 ; i < MAT_SIZE ; i++ ) {
			for ( j = 0 ; j < MAT_SIZE ; j++ ) {
				if( adj_mat[i * MAT_SIZE + j] > adj_mat[i * MAT_SIZE + k] + adj_mat[k * MAT_SIZE + j] )
					adj_mat[i * MAT_SIZE + j] = adj_mat[i * MAT_SIZE + k] + adj_mat[k * MAT_SIZE + j];
			}
		}
	}

	return 0;
}


static int floyd_warshall_module_callback(struct qhgpu_request *req) {
	int* adj_mat_gpu;

	printk("[floyd warshall kernel module]callback start.\n");

	//req->out have return data
	adj_mat_gpu = req->out;
	kprint_adjacency_matrix(adj_mat_gpu, 0x8);

	struct completion *c = (struct completion*)req->kdata;
	complete(c);

	return 0;
}


static int __init minit(void) {
	const unsigned int MAT_SIZE = 0x1000;

	int* adj_mat_cpu;
	int* adj_mat_gpu;

	struct qhgpu_request *req;
	char *buf;

	struct timeval t0, t1;
	long tt;

	printk("[floyd warshall kernel module]minit\n");


	//// alloc request
	req = qhgpu_alloc_request( MAT_SIZE*MAT_SIZE,"floyd_warshall_service");
	if (!req) {
		printk("request null\n");
		return 0;
	}

	req->callback = floyd_warshall_module_callback;

	// init mmap_addr
	adj_mat_gpu = req->kmmap_addr;

	//4096 * 2^16 == 2^2 * 2^10 * 2^16 == 2^26 * 2^2 == 0x1000*0x1000*2*sizeof(int)
	adj_mat_cpu = (int*)__get_free_pages(GFP_KERNEL, 16);
	kgenerate_adjacency_matrix(adj_mat_gpu, MAT_SIZE, 50);


	memcpy(adj_mat_cpu, adj_mat_gpu, MAT_SIZE*MAT_SIZE*sizeof(int));
	kprint_adjacency_matrix(adj_mat_gpu, 0x8);


	do_gettimeofday(&t0);
	//sync call
	qhgpu_call_sync(req);
	tt = 1000000*(t1.tv_sec-t0.tv_sec) +
			((long)(t1.tv_usec) - (long)(t0.tv_usec));
	printk("TIME: %10lu MS, OPS: %8lu\n", tt, 1000000/tt);


	do_gettimeofday(&t0);
	floyd_warshall_cpu(adj_mat_cpu, MAT_SIZE);
	do_gettimeofday(&t1);

	tt = 1000000*(t1.tv_sec-t0.tv_sec) +
			((long)(t1.tv_usec) - (long)(t0.tv_usec));

	printk("TIME: %10lu MS, OPS: %8lu\n", tt, 1000000/tt);

	printk("[floyd warshall kernel module]minit end.\n");

	return 0;
}

static void __exit mexit(void) {
	printk("[floyd warshall kernel module]mexit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
