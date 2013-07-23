#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include "connector.h"
#include "list.h"
#include "qhgpu.h"
#include "qhgpu_log.h"
#include "cl_operator.h"

#define ssc(...) _safe_syscall(__VA_ARGS__, __FILE__, __LINE__)

int _safe_syscall(int r, const char *file, int line) {
	if (r < 0) {
		fprintf(stderr, "Error in %s:%d, ", file, line);
		perror("");
		abort();
	}
	return r;
}

struct _qhgpu_sritem {
	struct qhgpu_service_request sr;
	struct list_head glist;
	struct list_head list;
};

static int devfd;

//use when mmap
#define PAGE_SIZE 4096
char* mmap_address;

struct qhgpu_gpu_mem_info hostbuf;
//struct qhgpu_gpu_mem_info hostvma;

volatile int qc_loop_continue = 1;

static char *service_lib_dir;
static char *qhgpu_dev_path;

/* lists of requests of different states */
LIST_HEAD(all_reqs);
LIST_HEAD(init_reqs);
LIST_HEAD(memdone_reqs);
LIST_HEAD(prepared_reqs);
LIST_HEAD(running_reqs);
LIST_HEAD(post_exec_reqs);
LIST_HEAD(done_reqs);

static int init_mmap() {
	mmap_address = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devfd, 0);
	if (mmap_address == MAP_FAILED) {
		printf("address map failed.\n");
		perror("mmap");
		return -1;
	}

	printf("init_mmap {%p}\n", mmap_address);

	return 0;
}

static int qc_init(void) {
	int i, len, r;
	void *p = NULL;

	printf("alloc GPU Pinned memory buffers : start \n");
	//dbg("alloc GPU Pinned memory buffers : start \n");
	//// device drive open
	//devfd = ssc(open(qhgpu_dev_path, O_RDWR));
	devfd = open(qhgpu_dev_path, O_RDWR);
	printf("open dev [%s], ret fd = %d\n", qhgpu_dev_path, devfd);

	// init memory map
	init_mmap();

	gpu_init();
	// alloc GPU Pinned memory buffers
	p = (void*) gpu_alloc_pinned_mem(QHGPU_BUF_SIZE + PAGE_SIZE);
	printf("alloc GPU Pinned memory buffers : %p \n", p);
	hostbuf.uva = p;
	hostbuf.size = QHGPU_BUF_SIZE;

	//dbg("alloc GPU Pinned memory buffers : %p \n", hostbuf.uva);
	memset(hostbuf.uva, 0, QHGPU_BUF_SIZE);
	printf("alloc GPU Pinned memory buffers : memset Done! \n");
	//ssc( mlock(hostbuf.uva, QHGPU_BUF_SIZE));
	ssc(mlock(hostbuf.uva, QHGPU_BUF_SIZE));

	printf("alloc GPU Pinned memory buffers : mlock Done! \n");

	len = sizeof(struct qhgpu_gpu_mem_info);
	printf("qhgpu_gpu_mem_info len  : %d", len);
	/**/
	/* tell kernel the buffers */
	r = ioctl(devfd, QHGPU_IOC_SET_GPU_BUFS, (unsigned long)&hostbuf);
	if (r < 0) {
		printf("device write fail!");
		perror("device write fail!");
		abort();
	}/**/

	return 0;
}

static int qc_finit(void) {
	int i;

	ioctl(devfd, QHGPU_IOC_SET_STOP);
	close(devfd);
	//gpu_finit();

	//gpu_free_pinned_mem(hostbuf.uva);

	return 0;
}

static struct _qhgpu_sritem *qc_alloc_service_request() {
	struct _qhgpu_sritem *s = (struct _qhgpu_sritem *) malloc(
			sizeof(struct _qhgpu_sritem));
	if (s) {
		memset(s, 0, sizeof(struct _qhgpu_sritem));
		INIT_LIST_HEAD(&s->list);
		INIT_LIST_HEAD(&s->glist);
	}
	return s;
}

static void qc_free_service_request(struct _qhgpu_sritem *s) {
	free(s);
}

static void qc_fail_request(struct _qhgpu_sritem *sreq, int serr) {
	sreq->sr.state = QHGPU_REQ_DONE;
	sreq->sr.errcode = serr;
	list_del(&sreq->list);
	list_add_tail(&sreq->list, &done_reqs);
}

static void qc_init_service_request(struct _qhgpu_sritem *item, struct qhgpu_ku_request *kureq) {
	//// add to all_reqs for request empty check
	list_add_tail(&item->glist, &all_reqs);

	memset(&item->sr, 0, sizeof(struct qhgpu_service_request));
	item->sr.id = kureq->id;
	item->sr.hin = kureq->in;
	item->sr.hout = kureq->out;
	item->sr.hdata = kureq->data;
	item->sr.insize = kureq->insize;
	item->sr.outsize = kureq->outsize;
	item->sr.datasize = kureq->datasize;
	item->sr.stream_id = -1;
	item->sr.s = qc_lookup_service(kureq->service_name);
	if (!item->sr.s) {
		printf("can't find service\n");
		qc_fail_request(item, QHGPU_NO_SERVICE);
	} else {
		item->sr.s->compute_size(&item->sr);				//// [?]
		item->sr.state = QHGPU_REQ_INIT;
		item->sr.errcode = 0;

		//// add to init_reqs
		list_add_tail(&item->list, &init_reqs);
	}
}

static int qc_get_next_service_request(void) {
	int err;
	struct pollfd pfd;

	struct _qhgpu_sritem *sreq;
	struct qhgpu_ku_request kureq;

	pfd.fd = devfd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	printf("qc_get_next_service_request start!!! \n");

	err = poll(&pfd, 1, list_empty(&all_reqs) ? -1 : 0);/// if list is empty polling (timeout -1)
														/// qhgpudev.reqq poll_wait interrupted by call
	printf("qc_get_next_service_request err: %d \n", err);

	if (err == 0 || (err && !(pfd.revents & POLLIN))) {
		return -1;
	} else if (err == 1 && pfd.revents & POLLIN) {
		//	_qhgpu_sritem allocation
		sreq = qc_alloc_service_request();
		if (!sreq)
			return -1;

		//// get request --> kureq
		err = read(devfd, (char*) (&kureq), sizeof(struct qhgpu_ku_request));

		if (err <= 0) {
			if (errno == EAGAIN || err == 0) {

				return -1;
			} else {
				perror("Read request.");
				abort();
			}
		} else {
			qc_init_service_request(sreq, &kureq);
			return 0;
		}
	} else {
		if (err < 0) {
			perror("Poll request");
			abort();
		} else {
			fprintf(stderr, "Poll returns multiple fd's results\n");
			abort();
		}
	}
	printf("qc_get_next_service_request finish!!");
}

static int qc_request_alloc_mem(struct _qhgpu_sritem *sreq) {

	printf("qc_request_alloc_mem !!!\n");
	int r = 0;

	///	alloc service on gpu memory
	/*
	 r = gpu_alloc_device_mem(&sreq->sr);
	 if (r) {
	 return -1;
	 } else {*/
	sreq->sr.state = QHGPU_REQ_MEM_DONE;
	list_del(&sreq->list);
	list_add_tail(&sreq->list, &memdone_reqs);

	return 0;
	//}
}

static int qc_prepare_exec(struct _qhgpu_sritem *sreq) {
	printf("qc_prepare_exec !!!\n");
	int r;

	/// alloc stream
	/*if (gpu_alloc_stream(&sreq->sr)) {
	 r = -1;
	 } else {

	 // copy host to device
	 r = sreq->sr.s->prepare(&sreq->sr);
	 if (r) {
	 printf("%d fails prepare\n", sreq->sr.id);
	 qc_fail_request(sreq, r);
	 } else {*/
	sreq->sr.state = QHGPU_REQ_PREPARED;
	list_del(&sreq->list);
	list_add_tail(&sreq->list, &prepared_reqs);
	//}
	//}
	return r;
}

static int qc_launch_exec(struct _qhgpu_sritem *sreq) {
	printf("qc_launch_exec !!!\n");
	//// GPU computation
	int r = sreq->sr.s->launch(&sreq->sr);
	if (r) {
		printf("%d fails launch\n", sreq->sr.id);
		qc_fail_request(sreq, r);
	} else {
		sreq->sr.state = QHGPU_REQ_RUNNING;
		list_del(&sreq->list);
		list_add_tail(&sreq->list, &running_reqs);
	}

	return 0;
}

static int qc_post_exec(struct _qhgpu_sritem *sreq) {
	int r = 1;
	printf("qc_post_exec !!!\n");

	//// check stream done
	/*if (gpu_execution_finished(&sreq->sr)) {

	 // copy result device to host
	 if (!(r = sreq->sr.s->post(&sreq->sr))) {*/
	sreq->sr.state = QHGPU_REQ_POST_EXEC;
	list_del(&sreq->list);
	list_add_tail(&sreq->list, &post_exec_reqs);
	/*	}
	 else {
	 printf("%d fails post\n", sreq->sr.id);
	 qc_fail_request(sreq, r);
	 }
	 }*/
	return r;
}

static int qc_finish_post(struct _qhgpu_sritem *sreq) {
	printf("qc_finish_post !!!\n");

	//// check stream done
	//if (gpu_post_finished(&sreq->sr)) {
	sreq->sr.state = QHGPU_REQ_DONE;
	list_del(&sreq->list);
	list_add_tail(&sreq->list, &done_reqs);

	return 0;
	//}
	//return 1;
}

////////////////////////////////////////////////////////////////////////////////////
// response
////////////////////////////////////////////////////////////////////////////////////
static int qc_send_response(struct qhgpu_ku_response *resp) {
	printf("qc_send_response: %d \n", resp->id);

	/// write result to device driver
	ssc(write(devfd, resp, sizeof(struct qhgpu_ku_response)));

	printf("mmap data : {%s}\n", mmap_address);

	return 0;
}

static int qc_service_done(struct _qhgpu_sritem *sreq) {
	struct qhgpu_ku_response resp;

	printf("qc_service_done !!!\n");

	resp.id = sreq->sr.id;
	resp.errcode = sreq->sr.errcode;

	qc_send_response(&resp);

	list_del(&sreq->list);
	list_del(&sreq->glist);

	qc_free_service_request(sreq);
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

static int __qc_process_request(int (*op)(struct _qhgpu_sritem *),
		struct list_head *lst, int once) {
	struct list_head *pos, *n;
	int r = 0;

	list_for_each_safe(pos, n, lst)
	{
		r = op(list_entry(pos, struct _qhgpu_sritem, list));
		if (!r && once)
			break;
	}
	return r;
}

static int qc_main_loop() {
	while (qc_loop_continue) {
		__qc_process_request(qc_service_done, &done_reqs, 0);
		__qc_process_request(qc_finish_post, &post_exec_reqs, 0);
		__qc_process_request(qc_post_exec, &running_reqs, 1);
		__qc_process_request(qc_launch_exec, &prepared_reqs, 1);
		__qc_process_request(qc_prepare_exec, &memdone_reqs, 1);
		__qc_process_request(qc_request_alloc_mem, &init_reqs, 0);

		qc_get_next_service_request();
	}
	return 0;
}

int main(int argc, char *argv[]) {
	int c;
	qhgpu_dev_path = "/dev/qhgpu";
	qc_init();
	qc_main_loop();
	qc_finit();
	return 0;
}
