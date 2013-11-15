#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netfilter_ipv4.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <sys/time.h>
//#include <openssl/md5.h>

#include "../../../qhgpu/qhgpu.h"
#include "../../../qhgpu/connector.h"
#include "cl_gpu_firewall.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <fcntl.h>

#define IFNAMSIZ 16

#define BUFSIZE 2048
#define PAYLOADSIZE 2048

#define SRC_ADDR(payload) (*(in_addr_t *)((payload)+12))
#define DST_ADDR(payload) (*(in_addr_t *)((payload)+16))

#define EXTRACT_32BITS(p) \
          ((u_int32_t)ntohl(((const unaligned_u_int32_t *)(p))))

#define TCP 6
#define UDP 17

struct gpu_firewall_data {
	cl_context context;
	cl_device_id device_id;
	cl_command_queue command_queue;
	cl_program program;
	cl_kernel kernel;
	cl_mem mem_obj;
};

struct gpu_firewall_data fw_data;

int off_flag;

//qhgpu form
// Default Process
int gpu_firewall_cs(struct qhgpu_service_request *sr) {
	printf("[libsrv_default] Info: gpu_firewall_cs\n");
	return 0;
}


int gpu_firewall_launch(struct qhgpu_service_request *sr)
{
	cl_int res;
	/*int* mmap_data;
	unsigned int mmap_size;

	double data_size;
	unsigned int mat_size;

	printf("[libsrv_gpu_firewall]gpu_firewall launch\n");

	mmap_data = sr->mmap_addr;
	mmap_size = sr->mmap_size;
	printf("[libsrv_gpu_firewall] mmap_addr[%p], mmap_size[%x]\n", mmap_data, mmap_size);

	data_size = mmap_size;
	mat_size = sqrt(data_size);

	res = run_gpu_firewall_gpu(&(fw_data.context), &(fw_data.command_queue), &(fw_data.kernel), &(fw_data.mem_obj), mmap_data, mat_size);
	if( res != CL_SUCCESS ) {
		printf("init_openCL error.\n");
		return 1;
	}

	clFinish(fw_data.command_queue);
	clReleaseMemObject(fw_data.mem_obj);*/

	if( off_flag == 0 ) {
		printf("packet_poll on s\n");
		system("./packet_poll &");
		printf("packet_poll on e\n");
		off_flag++;
	}
	else {
		printf("packet_poll off s\n");
		system("./killall packet_poll");
		printf("packet_poll off e\n");
	}

	return 0;
}

int gpu_firewall_post(struct qhgpu_service_request *sr)
{
	printf("[libsrv_gpu_firewall] Info: gpu_firewall_post\n");
	return 0;
}

static struct qhgpu_service gpu_firewall_srv;

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*), cl_context context, cl_device_id* device_id) {
	cl_int ret;

	printf("[libsrv_gpu_firewall] Info: init libsrv gpu firewall.\n");

	/*fw_data.context = context;
	fw_data.device_id = device_id;

	ret = init_gpu_firewall_gpu(device_id, &context, &(fw_data.command_queue), &(fw_data.program), &(fw_data.kernel));
	if( ret != CL_SUCCESS) {
		printf("init_gpu_firewall_gpu error.\n");
		return 1;
	}*/

	off_flag = 0;

	sprintf(gpu_firewall_srv.name, "gpu_firewall_service");
	gpu_firewall_srv.compute_size = gpu_firewall_cs;
	gpu_firewall_srv.launch = gpu_firewall_launch;
	gpu_firewall_srv.post = gpu_firewall_post;

	return reg_srv(&gpu_firewall_srv, lh);
}

int finit_service(void *lh, int (*unreg_srv)(const char*)) {
	printf("[libsrv_gpu_firewall] Info: finit libsrv gpu firewall.\n");

	/*clFinish(fw_data.command_queue);
	clReleaseKernel(fw_data.kernel);
	clReleaseProgram(fw_data.program);
	clReleaseCommandQueue(fw_data.command_queue);*/

	return unreg_srv(gpu_firewall_srv.name);
}
