/* This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/random.h>

#include "../../qhgpu/qhgpu.h"


#define __INT_MAX 9999999
//#define __INT_MAX 2147483647;


/********************************
 * generate non directional adjacency matrix
 * generate almost (rate/100)% nonzero edge
 ********************************/
void kgenerate_adjacency_matrix(unsigned int* mat, const unsigned int mat_size, unsigned int rate) {
	unsigned int i, j, temp;

	for(i = 0 ; i < mat_size ; i++ ) {
		mat[i*mat_size + i] = 0;
		for(j = i+1 ; j < mat_size ; j++ ) {
			get_random_bytes(&temp, sizeof(unsigned int));
			temp = temp % 10000 + 1;
			if( temp > rate * 100 ) {
				temp = __INT_MAX;
			}
			mat[j*mat_size + i] = mat[i*mat_size + j] = temp;
		}
	}

	/*//generate srand
	srand((unsigned)time(NULL)+(unsigned)getpid());

	for(i = 0 ; i < mat_size ; i++ ) {
		mat[i*mat_size + i] = 0;
		for(j = i+1 ; j < mat_size ; j++ ) {
			temp = rand() % 100 + 1;
			if( temp > rate ) {
				temp = __INT_MAX;
			}
			mat[j*mat_size + i] = mat[i*mat_size + j] = temp;
		}
	}*/
}


/********************************
 * print adjacency matrix
 ********************************/
void kprint_adjacency_matrix(unsigned int* adj_mat, const unsigned int MAT_SIZE) {
	int i, j;

	for(i = 0 ; i < 8 ; i++ ) {
		for(j = 0 ; j < 8 ; j++ ) {
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
	//req->out have return data
	struct completion *c;

	printk("[floyd warshall kernel module]callback start.\n");

	c = (struct completion*)req->kdata;
	complete(c);

	return 0;
}


static int __init minit(void) {
	const unsigned int MAT_SIZE = 0x400;

	int* adj_mat_cpu;
	int* adj_mat_gpu;

	struct qhgpu_request *req;

	struct timeval t0, t1;
	long tt;

	printk("[floyd warshall kernel module]minit\n");


	// alloc request
	//qhgpu_alloc_request( num_of_int , serv_name )  num_of_int == data_size / sizeof(int)
	req = qhgpu_alloc_request( MAT_SIZE*MAT_SIZE, "floyd_warshall_service");
	if (!req) {
		printk("request null\n");
		return 0;
	}

	req->callback = floyd_warshall_module_callback;

	// init mmap_addr
	adj_mat_gpu = (int*)req->kmmap_addr;

	//4096 * 2^16 == 2^2 * 2^10 * 2^16 == 2^26 * 2^2 == 0x1000*0x1000*2*sizeof(int)
	adj_mat_cpu = (int*)__get_free_pages(GFP_KERNEL, 10);
	kgenerate_adjacency_matrix(adj_mat_cpu, MAT_SIZE, 50);


	memcpy(adj_mat_gpu, adj_mat_cpu, MAT_SIZE*MAT_SIZE*sizeof(int));

	kprint_adjacency_matrix(adj_mat_gpu, MAT_SIZE);


	do_gettimeofday(&t0);

	//sync call
	qhgpu_call_sync(req);

	do_gettimeofday(&t1);
	tt = 1000000*(t1.tv_sec-t0.tv_sec) + ((long)(t1.tv_usec) - (long)(t0.tv_usec));
	printk("GPU TIME: %10lu MS\n", tt);

	kprint_adjacency_matrix(adj_mat_gpu, MAT_SIZE);

	do_gettimeofday(&t0);

	kfloyd_warshall_cpu(adj_mat_cpu, MAT_SIZE);

	do_gettimeofday(&t1);

	tt = 1000000*(t1.tv_sec-t0.tv_sec) + ((long)(t1.tv_usec) - (long)(t0.tv_usec));

	printk("CPU TIME: %10lu MS\n", tt);

	kprint_adjacency_matrix(adj_mat_cpu, MAT_SIZE);

	printk("[floyd warshall kernel module]minit end.\n");

	return 0;
}

static void __exit mexit(void) {
	printk("[floyd warshall kernel module]mexit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
