/* This work is licensed under the terms of the GNU GPL, version 2.  See
 * the GPL-COPYING file in the top-level directory.
 *
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "../../../qhgpu/qhgpu.h"
#include "../../../qhgpu/connector.h"
#include "cl_bfs.h"

typedef enum { false, true} bool;
typedef enum { PAGEABLE, PINNED, NON } memoryMode;

#define MAX_THREADS_PER_BLOCK 512
int work_group_size = 512;

struct Node {
	int starting;
	int no_of_edges;
};

void run_bfs_gpu(int no_of_nodes, struct Node *h_graph_nodes, int edge_list_size, \
		int *h_graph_edges, bool *h_graph_mask, bool *h_updating_graph_mask, \
		bool *h_graph_visited, int *h_cost)
{
	memoryMode memMode = PINNED;
	//memoryMode memMode = NON;
	//int number_elements = height*width;
	bool h_over;
	cl_mem d_graph_nodes, d_graph_edges, d_graph_mask, d_updating_graph_mask, \
		d_graph_visited, d_cost, d_over;
	//--1 transfer data from host to device

	if(memMode == PINNED)
	{
		printf("PINNED\n");
		d_graph_nodes = _clCreateAndCpyPinnedMem(no_of_nodes*sizeof(struct Node), h_graph_nodes);
		d_graph_edges = _clCreateAndCpyPinnedMem(edge_list_size*sizeof(int), h_graph_edges);
	}
	else if(memMode == PAGEABLE)
	{
		printf("PAGEABLE\n");
		d_graph_nodes = _clCreateAndCpyPagedMem(no_of_nodes*sizeof(struct Node), (float*)h_graph_nodes);
		d_graph_edges = _clCreateAndCpyPagedMem(edge_list_size*sizeof(int), (float*)h_graph_edges);
	}
	else
	{
		printf("non test\n");
		d_graph_nodes = _clMalloc(no_of_nodes*sizeof(struct Node), h_graph_nodes);
		d_graph_edges = _clMalloc(edge_list_size*sizeof(int), h_graph_edges);

		_clMemcpyH2D(d_graph_nodes, no_of_nodes*sizeof(struct Node), h_graph_nodes);
		_clMemcpyH2D(d_graph_edges, edge_list_size*sizeof(int), h_graph_edges);

	}

	d_graph_mask = _clMalloc(no_of_nodes*sizeof(bool), h_graph_mask);
	d_updating_graph_mask = _clMalloc(no_of_nodes*sizeof(bool), h_updating_graph_mask);
	d_graph_visited = _clMalloc(no_of_nodes*sizeof(bool), h_graph_visited);
	d_cost = _clMallocRW(no_of_nodes*sizeof(int), h_cost);
	d_over = _clMallocRW(sizeof(bool), &h_over);

	_clMemcpyH2D(d_graph_mask, no_of_nodes*sizeof(bool), h_graph_mask);
	_clMemcpyH2D(d_updating_graph_mask, no_of_nodes*sizeof(bool), h_updating_graph_mask);
	_clMemcpyH2D(d_graph_visited, no_of_nodes*sizeof(bool), h_graph_visited);
	_clMemcpyH2D(d_cost, no_of_nodes*sizeof(int), h_cost);

//--2 invoke kernel

	do
	{
		h_over = false;
		_clMemcpyH2D(d_over, sizeof(bool), &h_over);
		//--kernel 0
		int kernel_id = 0;
		int kernel_idx = 0;
		_clSetArgs(kernel_id, kernel_idx++, d_graph_nodes);
		_clSetArgs(kernel_id, kernel_idx++, d_graph_edges);
		_clSetArgs(kernel_id, kernel_idx++, d_graph_mask);
		_clSetArgs(kernel_id, kernel_idx++, d_updating_graph_mask);
		_clSetArgs(kernel_id, kernel_idx++, d_graph_visited);
		_clSetArgs(kernel_id, kernel_idx++, d_cost);
		_clSetArgs_1(kernel_id, kernel_idx++, &no_of_nodes, sizeof(int));

		//int work_items = no_of_nodes;
		_clInvokeKernel(kernel_id, no_of_nodes, work_group_size);

		//--kernel 1
		kernel_id = 1;
		kernel_idx = 0;
		_clSetArgs(kernel_id, kernel_idx++, d_graph_mask);
		_clSetArgs(kernel_id, kernel_idx++, d_updating_graph_mask);
		_clSetArgs(kernel_id, kernel_idx++, d_graph_visited);
		_clSetArgs(kernel_id, kernel_idx++, d_over);
		_clSetArgs_1(kernel_id, kernel_idx++, &no_of_nodes, sizeof(int));

		//work_items = no_of_nodes;
		_clInvokeKernel(kernel_id, no_of_nodes, work_group_size);

		_clMemcpyD2H(d_over,sizeof(bool), &h_over);

	}while(h_over);

	_clFinish();

	_clMemcpyD2H(d_cost,no_of_nodes*sizeof(int), h_cost);

	return ;
}

int bfs_cs(struct qhgpu_service_request *sr) {
	printf("[libsrv bfs] bfs cs\n");
	return 0;
}

int bfs_launch(struct qhgpu_service_request *sr) {
	unsigned int* mmap_data;
	unsigned int mmap_size;

	int no_of_nodes, i, data_index;
	int edge_list_size;

	struct Node * h_graph_nodes;
	bool *h_graph_mask, *h_updating_graph_mask, *h_graph_visited;
	int *h_graph_edges;
	int * h_cost;
	int * h_cost_ref;

	int source;
	int num_of_blocks;
	int num_of_threads_per_block;
	int start, edgeno;
	int id, cost;

	printf("[libsrv bfs]mmap_ioctl launch\n");

	//init mmap data
	mmap_data = sr->mmap_addr;
	mmap_size = sr->mmap_size;
	data_index = 0;

	//fscanf(fp, "%d", &no_of_nodes);
	no_of_nodes = mmap_data[data_index];
	data_index++;

	num_of_blocks = 1;
	num_of_threads_per_block = no_of_nodes;

	if(no_of_nodes > MAX_THREADS_PER_BLOCK) {
		num_of_blocks = (int)(no_of_nodes/(double)MAX_THREADS_PER_BLOCK);
		num_of_threads_per_block = MAX_THREADS_PER_BLOCK;
	}

	work_group_size = num_of_threads_per_block;
	h_graph_nodes = (struct Node*)malloc(sizeof(struct Node) * no_of_nodes);
	h_graph_mask = (bool*)malloc(sizeof(bool) * no_of_nodes);
	h_updating_graph_mask = (bool*)malloc(sizeof(bool) * no_of_nodes);
	h_graph_visited = (bool*)malloc(sizeof(bool) * no_of_nodes);


	for(i = 0 ; i < no_of_nodes; i++) {
		//fscanf(fp, "%d %d",&start, &edgeno);
		start = mmap_data[data_index];
		data_index++;
		no_of_nodes = mmap_data[data_index];
		edgeno++;

		h_graph_nodes[i].starting = start;
		h_graph_nodes[i].no_of_edges = edgeno;
		h_graph_mask[i] = false;
		h_updating_graph_mask[i] = false;
		h_graph_visited[i] = false;
	}

	//fscanf(fp, "%d", &source);
	source = mmap_data[data_index];
	data_index++;
	source = 0;

	h_graph_mask[source] = true;
	h_graph_visited[source] = true;
	//fscanf(fp, "%d", &edge_list_size);
	edge_list_size = mmap_data[data_index];
	data_index++;

	h_graph_edges = (int*)malloc(sizeof(int) * edge_list_size);

	for(i = 0; i < edge_list_size ; i++) {
		//fscanf(fp, "%d", &id);
		id = mmap_data[data_index];
		data_index++;
		//fscanf(fp, "%d", &cost);
		cost = mmap_data[data_index];
		data_index++;
		h_graph_edges[i] = id;
	}

	h_cost = (int *)malloc(sizeof(int) * no_of_nodes);
	h_cost_ref = (int *)malloc(sizeof(int) * no_of_nodes);

	for(i = 0; i < no_of_nodes ; i++){
		h_cost[i] = -1;
		h_cost_ref[i] = -1;
	}

	h_cost[source] = 0;
	h_cost_ref[source] = 0;

	run_bfs_gpu(no_of_nodes,h_graph_nodes,edge_list_size,h_graph_edges, h_graph_mask, h_updating_graph_mask, h_graph_visited, h_cost);

	/*for(i = 0; i < no_of_nodes; i++){
		h_graph_mask[i]=false;
		h_updating_graph_mask[i]=false;
		h_graph_visited[i]=false;
	}

	//set the source node as true in the mask
	source=0;
	h_graph_mask[source]=true;
	h_graph_visited[source]=true;
	run_bfs_cpu(no_of_nodes,h_graph_nodes,edge_list_size,h_graph_edges, h_graph_mask, h_updating_graph_mask, h_graph_visited, h_cost_ref);

	compare_results(h_cost, h_cost_ref, no_of_nodes);*/

	memcpy(mmap_data, h_cost, mmap_size * sizeof(int));

	//release host memory
	free(h_graph_nodes);
	free(h_graph_mask);
	free(h_updating_graph_mask);
	free(h_graph_visited);

	return 0;
}


int bfs_post(struct qhgpu_service_request *sr) {
	printf("[libsrv bfs] bfs_post.\n");
	return 0;
}

int bfs_prepare() {
	return 0;
}

static struct qhgpu_service bfs;

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*), cl_context ctx, cl_device_id* dv) {
	printf("[libsrv bfs] init bfs service.\n");

	cl_context context;
	cl_device_id dev = dv;

	context = ctx;
	dev  = dv;

	_clInit(context,dev);

	sprintf(bfs.name, "bfs_service");
	bfs.sid = 1;
	bfs.compute_size = bfs_cs;
	bfs.launch = bfs_launch;
	bfs.post = bfs_post;

	return reg_srv(&bfs, lh);
}

int finit_service(void *lh, int (*unreg_srv)(const char*)) {
	printf("[libsrv bfs] finit bfs service.\n");
	return unreg_srv(bfs.name);
}
