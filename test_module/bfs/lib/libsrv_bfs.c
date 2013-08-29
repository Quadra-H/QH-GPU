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

////////////////////////////////////////////////////////////////
const char *input_f = "../data/graph1MW_6.txt";
////////////////////////////////////////////////////////////////


typedef enum { false, true} bool;
typedef enum { PAGEABLE, PINNED, NON } memoryMode;

#define MAX_THREADS_PER_BLOCK 512
int work_group_size = 512;

struct Node
{
	int starting;
	int no_of_edges;
};

double gettime() {
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec+t.tv_usec*1e-6;
}

void compare_results(const int *gpu_results, const int *cpu_results, const int size)
{
    bool passed = true;
#pragma omp parallel for
    int i;
    for (i=0; i<size; i++)
    {
      if (cpu_results[i]!=gpu_results[i]){
         passed = false;
      }
    }
    if (passed)
    {
        printf ("--cambine:passed:-)");

    }
    else
    {
    	printf( "--cambine: failed:-(");
    }
    return ;
}

void run_bfs_cpu(int no_of_nodes, struct Node *h_graph_nodes, int edge_list_size, \
		int *h_graph_edges, bool *h_graph_mask, bool *h_updating_graph_mask, \
		bool *h_graph_visited, int *h_cost_ref){
	bool stop;
	int k = 0;
	int tid, i ;

	double time = 0;
	double t1 = gettime();

	do
	{
		stop=false;
		for(tid= 0; tid < no_of_nodes; tid++ )
		{
			if (h_graph_mask[tid] == true)
			{
				h_graph_mask[tid]=false;
				for(i = h_graph_nodes[tid].starting; i<(h_graph_nodes[tid].no_of_edges + h_graph_nodes[tid].starting); i++)
				{
					int id = h_graph_edges[i];

					if(!h_graph_visited[id])
					{
						h_cost_ref[id]=h_cost_ref[tid]+1;
						h_updating_graph_mask[id]=true;
					}
				}
			}
		}

  		for(tid=0; tid< no_of_nodes ; tid++ )
		{
			if (h_updating_graph_mask[tid] == true){
			h_graph_mask[tid]=true;
			h_graph_visited[tid]=true;
			stop=true;
			h_updating_graph_mask[tid]=false;
			}
		}
		k++;
	}while(stop);

	double t2 = gettime();
	time += t2 - t1;

	printf("Process time of CPU : %lf\n", time);
}

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


	double time = 0;
	double t1 = gettime();
	t1 = gettime();

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

	double t2 = gettime();
	time += t2 - t1;

	printf("Process time of GPU : %lf\n", time);
	return ;
}

int bfs_cs(struct qhgpu_service_request *sr)
{
	printf("[libsrv_default] Info: default_cs\n");
	return 0;
}

// launch and copy result to sr->dout
int bfs_launch(struct qhgpu_service_request *sr)
{

	int i=0;

	printf("\n\n init bfs =====\n");

	int no_of_nodes;
	int edge_list_size;
	FILE * fp;

	struct Node * h_graph_nodes;
	bool *h_graph_mask, *h_updating_graph_mask, *h_graph_visited;

	printf("Reading File...\n");


	fp = fopen(input_f, "r");
	if(!fp)
	{
		printf("Error Reading graph file\n");
		return 0;
	}


	int source = 0;
	fscanf(fp, "%d", &no_of_nodes);

	int num_of_blocks = 1;
	int num_of_threads_per_block = no_of_nodes;

	if(no_of_nodes > MAX_THREADS_PER_BLOCK)
	{
		num_of_blocks = (int)(no_of_nodes/(double)MAX_THREADS_PER_BLOCK);
		num_of_threads_per_block = MAX_THREADS_PER_BLOCK;
	}

	work_group_size = num_of_threads_per_block;
	h_graph_nodes = (struct Node*)malloc(sizeof(struct Node) * no_of_nodes);
	h_graph_mask = (bool*)malloc(sizeof(bool) * no_of_nodes);
	h_updating_graph_mask = (bool*)malloc(sizeof(bool) * no_of_nodes);
	h_graph_visited = (bool*)malloc(sizeof(bool) * no_of_nodes);

	int start, edgeno;

	for(i = 0 ; i < no_of_nodes; i++)
	{
		fscanf(fp, "%d %d",&start, &edgeno);
		h_graph_nodes[i].starting = start;
		h_graph_nodes[i].no_of_edges = edgeno;
		h_graph_mask[i] = false;
		h_updating_graph_mask[i] = false;
		h_graph_visited[i] = false;
	}

	fscanf(fp, "%d", &source);
	source = 0;

	h_graph_mask[source] = true;
	h_graph_visited[source] = true;
	fscanf(fp, "%d", &edge_list_size);
	int id, cost;
	int *h_graph_edges = (int*)malloc(sizeof(int) * edge_list_size);

	for(i = 0; i < edge_list_size ; i++)
	{
		fscanf(fp, "%d", &id);
		fscanf(fp, "%d", &cost);
		h_graph_edges[i] = id;
	}

	if(fp)
		fclose(fp);

	int * h_cost = (int *)malloc(sizeof(int) * no_of_nodes);
	int * h_cost_ref = (int *)malloc(sizeof(int) * no_of_nodes);

	for(i = 0; i < no_of_nodes ; i++){
		h_cost[i] = -1;
		h_cost_ref[i] = -1;
	}

	h_cost[source] = 0;
	h_cost_ref[source] = 0;

	run_bfs_gpu(no_of_nodes,h_graph_nodes,edge_list_size,h_graph_edges, h_graph_mask, h_updating_graph_mask, h_graph_visited, h_cost);

	for(i = 0; i < no_of_nodes; i++){
		h_graph_mask[i]=false;
		h_updating_graph_mask[i]=false;
		h_graph_visited[i]=false;
	}
	//set the source node as true in the mask
	source=0;
	h_graph_mask[source]=true;
	h_graph_visited[source]=true;
	run_bfs_cpu(no_of_nodes,h_graph_nodes,edge_list_size,h_graph_edges, h_graph_mask, h_updating_graph_mask, h_graph_visited, h_cost_ref);

	compare_results(h_cost, h_cost_ref, no_of_nodes);
	//release host memory
	free(h_graph_nodes);
	free(h_graph_mask);
	free(h_updating_graph_mask);
	free(h_graph_visited);
	return 0;
}


int bfs_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_bfs] Info: bfs_post\n");
	return 0;
}

int bfs_prepare()
{

}

static struct qhgpu_service bfs;

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*),
		cl_context ctx,
		cl_device_id* dv)
{
	printf("[libsrv_bfs] Info: init bfs service !!!!\n");



	context= ctx;
	dev  = dv;

	/////////////////////////////////////////////
	//bfs init
	/////////////////////////////////////////////
	_clInit(context,dev);
	/////////////////////////////////////////////



	sprintf(bfs.name, "libsrv_bfs");
	bfs.sid = 1;
	bfs.compute_size = bfs_cs;
	bfs.launch = bfs_launch;
	bfs.post = bfs_post;


	return reg_srv(&bfs, lh);
}

int finit_service(void *lh, int (*unreg_srv)(const char*))
{
	printf("[libsrv_default] Info: finit test service\n");
	return unreg_srv(bfs.name);
}


