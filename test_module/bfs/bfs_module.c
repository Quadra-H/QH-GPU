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

struct Node {
	int starting;
	int no_of_edges;
};

static int bfs_module_callback(struct qhgpu_request *req) {
	//req->out have return data
	struct completion *c;

	printk("[bfs kernel module]callback.\n");

	c = (struct completion*)req->kdata;
	complete(c);

	return 0;
}


/********************************
 * generate non directional adjacency matrix
 * generate almost (rate/100)% nonzero edge
 ********************************/
void kgenerate_adjacency_matrix(unsigned int* mat, const unsigned int mat_size, const unsigned int rate) {
	unsigned int i, j, temp;

	for(i = 0 ; i < mat_size ; i++ ) {
		mat[i*mat_size + i] = 0;
		for(j = i+1 ; j < mat_size ; j++ ) {
			get_random_bytes(&temp, sizeof(unsigned int));
			temp = temp % 10000 + 1;
			if( temp > rate * 100 ) {
				temp = 0;
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

	for(i = 0 ; i < 8 ; i++ ) {
		for(j = 0 ; j < 8 ; j++ ) {
			printk("%8u", adj_mat[i*MAT_SIZE + j]);
		}
		printk("\n");
	}
	printk("\n");
}

//transform adjacency matrix to example form in memory
//return datasize
unsigned int convert_to_exd(unsigned int* mat, const unsigned int mat_size, unsigned int* data) {
	int i, j, count, sum;
	int data_index;

	sum = 0;
	data_index = 0;

	//fprintf(fp ,"%d\n", mat_size);
	data[data_index] = mat_size;
	data_index++;

	for (i = 0; i < mat_size; i++) {
		count = 0;
		for (j = 0; j < mat_size; j++) {
			if (mat[i * mat_size + j] != 0 ) {
				count++;
			}
		}

		//fprintf(fp, "%f %f\n", sum, count);
		data[data_index] = sum;
		data_index++;
		data[data_index] = count;
		data_index++;
		sum += count;
	}

	//fprintf(fp, "\n0\n\n%d\n", sum);
	data[data_index] = 0;
	data_index++;
	data[data_index] = sum;
	data_index++;

	for (i = 0; i < mat_size; i++) {
		for (j = 0 ; j < mat_size; j++) {
			if ( mat[i * mat_size + j] != 0) {
				//fprintf(fp, "%f %f\n", j, mat[i * mat_size + j]);
				data[data_index] = j;
				data_index++;
				data[data_index] = mat[i * mat_size + j];
				data_index++;
				if( data_index > mat_size * mat_size - 1 ) {
					printk("out of size\n");
					return -1;
				}
			}
		}
	}

	return data_index;
}

void compare_results(const int *gpu_results, const int *cpu_results, const int size) {
	bool passed = true;
	int i;
	for (i = 0; i < size; i++) {
		//printk("%d , %d \n",cpu_results[i],  gpu_results[i]);
		if (cpu_results[i] != gpu_results[i]) {
			passed = false;
		}
	}
	if (passed) {
		printk("--cambine: passed:-)\n");

	} else {
		printk("--cambine: failed:-(\n");
	}
	return;
}


void run_bfs_cpu(int no_of_nodes, struct Node *h_graph_nodes, int edge_list_size, int *h_graph_edges, bool *h_graph_mask, bool *h_updating_graph_mask, bool *h_graph_visited, int *h_cost_ref) {
	bool stop;
	int k = 0;
	int tid, i;

	do {
		stop = false;
		for (tid = 0; tid < no_of_nodes; tid++) {
			if (h_graph_mask[tid] == true) {
				h_graph_mask[tid] = false;
				for(i = h_graph_nodes[tid].starting ; i < (h_graph_nodes[tid].no_of_edges + h_graph_nodes[tid].starting) ; i++) {
					int id = h_graph_edges[i];

					if (!h_graph_visited[id]) {
						h_cost_ref[id] = h_cost_ref[tid] + 1;
						h_updating_graph_mask[id] = true;
					}
				}
			}
		}

		for (tid = 0; tid < no_of_nodes; tid++) {
			if (h_updating_graph_mask[tid] == true) {
				h_graph_mask[tid] = true;
				h_graph_visited[tid] = true;
				stop = true;
				h_updating_graph_mask[tid] = false;
			}
		}
		k++;
	} while (stop);
}



static int __init minit(void) {
	const unsigned int MAT_SIZE = 0x400;
	unsigned int data_size;
	unsigned int edge_list_size;

	//bfs vals
	int start, edgeno, data_index, i;
	int id, cost;
	int source;

	unsigned int* adj_mat;
	//will set mmap
	unsigned int* data;

	struct qhgpu_request *req;

	struct timeval t0, t1;
	long tt;

	struct Node *h_graph_nodes;
	int *h_graph_edges;
	bool *h_graph_mask, *h_updating_graph_mask, *h_graph_visited;
	int *h_cost;
	int *h_cost_ref;


	printk("[bfs kernel module]minit\n");

	// alloc request
	//qhgpu_alloc_request( num_of_int , serv_name )  num_of_int == data_size / sizeof(int)
	req = qhgpu_alloc_request( MAT_SIZE*MAT_SIZE, "bfs_service");
	if (!req) {
		printk("request null\n");
		return 0;
	}

	req->callback = bfs_module_callback;

	// set data mmap
	data = (unsigned int*)req->kmmap_addr;

	//max MAT_SIZE == 0x400
	//0x400 == 2^10  0x400^2 == sizeof(int)*2^10*2^10
	//alloc adj_mat
	adj_mat = (unsigned int*)__get_free_pages(GFP_KERNEL, 10);
	data_index = 0;

	//generate adj mat
	kgenerate_adjacency_matrix(adj_mat, MAT_SIZE, 20);
	kprint_adjacency_matrix(adj_mat, MAT_SIZE);

	//convert to example data
	data_size = convert_to_exd(adj_mat, MAT_SIZE, data);
	edge_list_size = data_size - 3 - MAT_SIZE;

	//timer start here BC mmap data modified in convert_to_exd
	do_gettimeofday(&t0);

	h_graph_nodes = (struct Node *)__get_free_pages(GFP_KERNEL, 10);
	h_graph_edges = (int *)__get_free_pages(GFP_KERNEL, 10);
	h_graph_mask = (bool*)__get_free_pages(GFP_KERNEL, 10);
	h_updating_graph_mask = (bool*)__get_free_pages(GFP_KERNEL, 10);
	h_graph_visited = (bool*)__get_free_pages(GFP_KERNEL, 10);
	h_cost = (int *)__get_free_pages(GFP_KERNEL, 10);
	h_cost_ref = (int *)__get_free_pages(GFP_KERNEL, 10);

	data_index++;
	//input data
	for(i = 0 ; i < MAT_SIZE ; i++) {
		//fscanf(fp, "%d %d",&start, &edgeno);
		start = data[data_index];
		data_index++;
		edgeno = data[data_index];
		data_index++;

		h_graph_nodes[i].starting = start;
		h_graph_nodes[i].no_of_edges = edgeno;
		h_graph_mask[i] = false;
		h_updating_graph_mask[i] = false;
		h_graph_visited[i] = false;
	}

	//fscanf(fp, "%d", &source);
	source = data[data_index];
	data_index++;
	source = 0;

	h_graph_mask[source] = true;
	h_graph_visited[source] = true;

	//fscanf(fp, "%d", &edge_list_size);
	edge_list_size = data[data_index];
	data_index++;

	for(i = 0; i < edge_list_size ; i++) {
		//fscanf(fp, "%d", &id);
		//fscanf(fp, "%d", &cost);
		id = data[data_index];
		data_index++;
		cost = data[data_index];
		data_index++;

		h_graph_edges[i] = id;
	}

	for(i = 0; i < MAT_SIZE ; i++) {
		h_cost[i] = -1;
		h_cost_ref[i] = -1;
	}

	h_cost[source] = 0;
	h_cost_ref[source] = 0;

	//bfs cpu
	run_bfs_cpu(MAT_SIZE, h_graph_nodes, edge_list_size, h_graph_edges, h_graph_mask, h_updating_graph_mask, h_graph_visited, h_cost_ref);

	do_gettimeofday(&t1);
	tt = 1000000*(t1.tv_sec-t0.tv_sec) + ((long)(t1.tv_usec) - (long)(t0.tv_usec));
	printk("CPU TIME: %10lu MS\n", tt);


	do_gettimeofday(&t0);

	//bfs gpu srv call
	qhgpu_call_sync(req);

	do_gettimeofday(&t1);

	tt = 1000000*(t1.tv_sec-t0.tv_sec) + ((long)(t1.tv_usec) - (long)(t0.tv_usec));
	printk("GPU TIME: %10lu MS\n", tt);

	h_cost = (int*)data;

	compare_results(h_cost, h_cost_ref, MAT_SIZE);

	free_pages((long unsigned int)h_graph_nodes, 10);
	free_pages((long unsigned int)h_graph_edges, 10);
	free_pages((long unsigned int)h_graph_mask, 10);
	free_pages((long unsigned int)h_updating_graph_mask, 10);
	free_pages((long unsigned int)h_graph_visited, 10);
	free_pages((long unsigned int)h_cost, 10);
	free_pages((long unsigned int)h_cost_ref, 10);

	printk("[bfs kernel module]minit end.\n");

	return 0;
}

static void __exit mexit(void) {
	printk("[floyd warshall kernel module]mexit\n");
}

module_init( minit);
module_exit( mexit);

MODULE_LICENSE("GPL");
