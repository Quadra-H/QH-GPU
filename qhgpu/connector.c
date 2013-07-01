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
	//struct qhgpu_service_request sr;
	struct list_head glist;
	struct list_head list;
};

static int devfd;

struct qhgpu_gpu_mem_info hostbuf;
struct qhgpu_gpu_mem_info hostvma;

volatile int qc_loop_continue = 1;

static char *service_lib_dir;
static char *qhgpudev;

/* lists of requests of different states */
LIST_HEAD(all_reqs);



//#define ssc(...) _safe_syscall(__VA_ARGS__, __FILE__, __LINE__)
static int qc_init(void)
{
	int  i, len, r;
	void *p;

	printf("alloc GPU Pinned memory buffers : start \n");
	//dbg("alloc GPU Pinned memory buffers : start \n");
	//// device drive open
	//devfd = ssc(open(qhgpudev, O_RDWR));
	devfd = ssc(open(qhgpudev, O_RDWR));
	printf("open dev [%s], ret fd = %d\n", qhgpudev, devfd);

	/* alloc GPU Pinned memory buffers */
	//p = (void*)gpu_alloc_pinned_mem(QHGPU_BUF_SIZE+PAGE_SIZE);
	/*hostbuf.uva = p;
	hostbuf.size = QHGPU_BUF_SIZE;
	printf("alloc GPU Pinned memory buffers : %p \n", hostbuf.uva);
	//dbg("alloc GPU Pinned memory buffers : %p \n", hostbuf.uva);
	memset(hostbuf.uva, 0, QHGPU_BUF_SIZE);
	printf("alloc GPU Pinned memory buffers : memset Done! \n");
	//ssc( mlock(hostbuf.uva, QHGPU_BUF_SIZE));
	mlock(hostbuf.uva, QHGPU_BUF_SIZE);
mmap
	printf("alloc GPU Pinned memory buffers : mlock Done! \n");*/

	//// #GPU# initialize
	//gpu_init();
	////////////////////////

	//devfd = open(qhgpudev, O_RDWR);
	/*printf("mmap start~!");
	hostvma.uva = (void*)mmap(
			NULL, QHGPU_BUF_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, devfd, 0);

	printf("mmap done !");

	hostvma.size = QHGPU_BUF_SIZE;mmap
	if (hostvma.uva == MAP_FAILED) {
		//qc_log(QH_LOG_ERROR,"set up mmap area failed\n");
		//perror("mmap for GPU");
		printf("mmap fail !! \n");
		abort();
	}
	//qc_log(QH_LOG_PRINT,"mmap start 0x%lX\n", hostvma.uva);

	len = sizeof(struct qhgpu_gpu_mem_info);
	printf("qhgpu_gpu_mem_info len  : %d",len);
*/
	/* tell kernel the buffers */
	/*r = ioctl(devfd, QHGPU_IOC_SET_GPU_BUFS, (unsigned long)&hostbuf);
	if (r < 0) {
		printf("device write fail!");
		perror("device write fail!");
		abort();
	}*/

	return 0;
}
static int qc_finit(void)
{
    int i;

    ioctl(devfd, QHGPU_IOC_SET_STOP);
    close(devfd);
    //gpu_finit();

    //gpu_free_pinned_mem(hostbuf.uva);

    return 0;
}






static int qc_get_next_service_request(void)
{
	int err;
	struct pollfd pfd;

	struct _qhgpu_sritem *sreq;
	struct qhgpu_ku_request kureq;

	pfd.fd = devfd;
	pfd.events = POLLIN;
	pfd.revents = 0;



	err = poll(&pfd, 1, list_empty(&all_reqs)? -1:0);


	if (err == 0 || (err && !(pfd.revents & POLLIN)) ) {
		return -1;
	}
	else if (err == 1 && pfd.revents & POLLIN)
	{
		//sreq = kh_alloc_service_request();
		if (!sreq)
			return -1;

		err = read(devfd, (char*)(&kureq), sizeof(struct qhgpu_ku_request));
		printf("qc_get_next_service_request err: %d \n",err);
		if (err <= 0) {
			if (errno == EAGAIN || err == 0) {

				return -1;
			} else {
				perror("Read request.");
				abort();
			}
		} else {

			printf("init service \n");
			//qc_log(QH_LOG_PRINT,"init service \n");
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






static int __qc_process_request(int (*op)(struct _kgpu_sritem *),
			      struct list_head *lst, int once)
{
	struct list_head *pos, *n;
	int r = 0;
	//qc_log(QH_LOG_PRINT,"__qc_process_request start \n");
	list_for_each_safe(pos, n, lst) {
		//qc_log(QH_LOG_PRINT,"__qc_process_request list !! \n");

	}
    return r;
}

static int qc_main_loop()
{
	while (qc_loop_continue)
	{
		qc_get_next_service_request();
    }

    return 0;
}



int main(int argc, char *argv[])
{
    int c;
	qhgpudev = "/dev/qhgpu";

    qc_init();
    qc_main_loop();
    qc_finit();
    return 0;
}







