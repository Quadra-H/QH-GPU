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
	cl_device_id* device_id;
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


int gpu_firewall_launch(struct qhgpu_service_request *sr) {
	printf("[libsrv_gpu_firewall] Info: gpu_firewall_launch\n");

	if( off_flag == 0 ) {
		printf("[libsrv_gpu_firewall] Info: packet_pool on\n");
		system("./packet_poll &");
		off_flag++;
	}
	else {
		printf("[libsrv_gpu_firewall] Info: packet_pool off\n");
		system("killall packet_poll");
		off_flag = 0;
	}

	printf("[libsrv_gpu_firewall] Info: gpu_firewall_launch end\n");

	return 0;
}

int gpu_firewall_post(struct qhgpu_service_request *sr) {
	printf("[libsrv_gpu_firewall] Info: gpu_firewall_post\n");
	return 0;
}

static struct qhgpu_service gpu_firewall_srv;

int do_work(int* packet_buff, int packet_buff_size) {
	return run_gpu_firewall(&(fw_data.context), &(fw_data.command_queue), &(fw_data.kernel), &(fw_data.mem_obj), (void*)packet_buff, packet_buff_size );
}

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*), cl_context context, cl_device_id* device_id) {
	cl_int ret;

	printf("[libsrv_gpu_firewall] Info: init libsrv gpu firewall.\n");

	fw_data.context = context;
	fw_data.device_id = device_id;

	ret = init_gpu_firewall(fw_data.device_id, &context, &(fw_data.command_queue), &(fw_data.program), &(fw_data.kernel));
	if( ret != CL_SUCCESS) {
		printf("init_gpu_firewall_gpu error.\n");
		return 1;
	}

	off_flag = 0;

	sprintf(gpu_firewall_srv.name, "gpu_firewall_service");
	gpu_firewall_srv.compute_size = gpu_firewall_cs;
	gpu_firewall_srv.launch = gpu_firewall_launch;
	gpu_firewall_srv.post = gpu_firewall_post;

	return reg_srv(&gpu_firewall_srv, lh);
}

int finit_service(void *lh, int (*unreg_srv)(const char*)) {
	printf("[libsrv_gpu_firewall] Info: finit libsrv gpu firewall.\n");

	clFinish(fw_data.command_queue);
	clReleaseKernel(fw_data.kernel);
	clReleaseProgram(fw_data.program);
	clReleaseCommandQueue(fw_data.command_queue);

	return unreg_srv(gpu_firewall_srv.name);
}
