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

#define UPACKETBUFFSIZE 128

#define SRC_ADDR(payload) (*(in_addr_t *)((payload)+12))
#define DST_ADDR(payload) (*(in_addr_t *)((payload)+16))

#define EXTRACT_32BITS(p) \
          ((u_int32_t)ntohl(((const unaligned_u_int32_t *)(p))))

#define TCP 6
#define UDP 17

static int id_buff[UPACKETBUFFSIZE];
static int sip_buff[UPACKETBUFFSIZE];
static int dip_buff[UPACKETBUFFSIZE];
static unsigned short sport_buff[UPACKETBUFFSIZE];
static unsigned short dport_buff[UPACKETBUFFSIZE];
static unsigned int u_packet_buff_index;
static long first_rv_time;
static long perfo_rv_time;

struct queued_pkt {
	uint32_t packet_id;
	char indev[IFNAMSIZ];
	char physindev[IFNAMSIZ];
	char outdev[IFNAMSIZ];
	char physoutdev[IFNAMSIZ];
	u_int32_t mark;
	time_t timestamp;
	unsigned char *payload;
	int payload_len;
};

struct gpu_firewall_data {
	cl_context context;
	cl_device_id* device_id;
	cl_command_queue command_queue;
	cl_program program;
	cl_kernel kernel;
	cl_mem mem_obj;
};
struct gpu_firewall_data fw_data;

void print_ip(int ip) {
	unsigned char bytes[4];
	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;
	printf("%3d.%3d.%3d.%3d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

int show_packet(unsigned char *dgram, unsigned int datalen) {
	struct iphdr *iphdrs;
	struct tcphdr *tcphdrs;
	struct udphdr *udphdrs;
	/*struct in_addr addr;
	uint32_t seq, ack;

	char md5msg[40];
	char source[64];
	int len;

	char *show_data;*/

	iphdrs = (struct iphdr *) dgram;

	switch (iphdrs->protocol) {
	case TCP:
		tcphdrs = (struct tcphdrs *) (dgram + sizeof(struct iphdr));

		sip_buff[u_packet_buff_index] = iphdrs->saddr;
		dip_buff[u_packet_buff_index] = iphdrs->daddr;
		sport_buff[u_packet_buff_index] = htons(tcphdrs->source);
		dport_buff[u_packet_buff_index] = htons(tcphdrs->dest);

		/*printf("TCP saddr[");
		print_ip(iphdrs->saddr);
		printf("]raddr[");
		print_ip(iphdrs->daddr);
		printf("]\n");
		printf("TCP port sport : %u \t dport : %u\n", htons(tcphdrs->source), htons(tcphdrs->dest));

		*//*
		 tcphdrs = (struct tcphdr *)(dgram + 4 * iphdrs->ihl);
		 show_data = (char *)(dgram + datalen - 40);
		 printf("%s\n", show_data);
		 */
		break;

	case UDP:
		udphdrs = (struct udphdr *) (dgram + sizeof(struct iphdr));

		sip_buff[u_packet_buff_index] = iphdrs->saddr;
		dip_buff[u_packet_buff_index] = iphdrs->daddr;
		sport_buff[u_packet_buff_index] = htons(udphdrs->source);
		dport_buff[u_packet_buff_index] = htons(udphdrs->dest);

		/*printf("UDP saddr[");
		print_ip(iphdrs->saddr);
		printf("]raddr[");
		print_ip(iphdrs->daddr);
		printf("]\n");
		printf("UDP port sport : %u \t dport : %u\n", htons(udphdrs->source), htons(udphdrs->dest));
		printf("UDP size : %u\n", htons(udphdrs->len));*/

		//show_data = (char *) (dgram + datalen - sizeof(struct iphdr) - sizeof(struct udphdr));

		break;

	default:
		printf("protocol num %d is unknown protocol\n", iphdrs->protocol);
		break;
	}

	return NF_ACCEPT;
}

static int packet_buffering(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
								struct nfq_data *nfa, void *data) {
	int id = 0, status = 0;
	struct queued_pkt q_pkt;
	struct nfqnl_msg_packet_hdr *ph;
	struct iphdr *iph = NULL;
	struct timeval timestamp;
	unsigned char *payload;
	int payload_len;
	int ret;
	u_int32_t POLICY;

	unsigned int i;
	struct timeval tv;
	long rv_time;

	struct iphdr *iphdrs;

#ifdef HAVE_NFQ_INDEV_NAME
	struct nlif_handle *nlif_handle = (struct nlif_handle *) data;
#endif


#ifdef HAVE_NFQ_INDEV_NAME
	if (!get_interface_information(nlif_handle, &q_pkt, nfa)) {
		log_area_printf(DEBUG_AREA_PACKET, DEBUG_LEVEL_INFO,
				"Can not get interfaces information for message");
		return 0;
	}

#else
	snprintf(q_pkt.indev, sizeof(q_pkt.indev), "*");
	snprintf(q_pkt.physindev, sizeof(q_pkt.physindev), "*");
	snprintf(q_pkt.outdev, sizeof(q_pkt.outdev), "*");
	snprintf(q_pkt.physoutdev, sizeof(q_pkt.physoutdev), "*");
#endif

	gettimeofday (&tv, NULL);
	rv_time = (tv.tv_sec * 1000 + tv.tv_usec / 1000);

	ph = nfq_get_msg_packet_hdr(nfa);

	if (ph) {
		id = ntohl(ph->packet_id);

		if( id % 1000 == 0 ) {
			printf("[libsrv_gpu_firewall] Info: 1000 packet compute time [%lu]usec\n", perfo_rv_time);
			perfo_rv_time = 0;
		}

		id_buff[u_packet_buff_index] = id;
		payload_len = nfq_get_payload(nfa, &payload);

		show_packet(payload, payload_len);

		u_packet_buff_index++;

		if( u_packet_buff_index == UPACKETBUFFSIZE || rv_time - first_rv_time > 1000 /*1000ms*/ ) {
			do_work(sip_buff, u_packet_buff_index );

			for( i = 0 ; i < u_packet_buff_index ; i++ ) {
				if( sip_buff[i] == 0 ) {
					nfq_set_verdict(qh, id_buff[i], NF_ACCEPT, 0, NULL);
				}
				else {
					nfq_set_verdict(qh, id_buff[i], NF_DROP, 0, NULL);
				}
			}

			u_packet_buff_index = 0;
			first_rv_time = rv_time;
		}
	}

	gettimeofday (&tv, NULL);
	perfo_rv_time += (tv.tv_sec * 1000 + tv.tv_usec / 1000) - rv_time;
}

_Bool break_flag = 0;
struct nfq_handle *h;
struct nfq_q_handle *qh;
struct nfnl_handle *nh;
int fd, rv;
char buf[BUFSIZE];
void handle_packet();
pthread_t exec_thread;
int thr_id;
int r;

int netlink_init(int q_num) {
	u_packet_buff_index = 0;

	h = nfq_open();
	if (!h) {
		fprintf(stderr, "Error During nfq_open()\n");
		exit(-1);
	}

	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "Error During nfq_unbind_pf()\n");
	}

	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "Error During nfq_bind_pf()\n");
		exit(-1);
	}

	qh = nfq_create_queue(h, q_num, &packet_buffering, NULL);
	if (!qh) {
		fprintf(stderr, "Error During nfq_create_queue()\n");
		exit(-1);
	}

	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, PAYLOADSIZE) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(-1);
	}

	fprintf(stderr, "NFQUEUE:binding from queue\n");

	nh = nfq_nfnlh(h);
	fd = nfnl_fd(nh);

	thr_id = pthread_create(&exec_thread, NULL, handle_packet, NULL);

	return (0);
}

void handle_packet(){
	printf("[libsrv_default] Info: gpu_firewall_cs\n");
	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0 && !break_flag) {
		nfq_handle_packet(h, buf, rv);
	}
	printf("[libsrv_default] Info: gpu_firewall_cs\n");
}

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
		netlink_init(0);
		//system("./packet_poll &");
		off_flag++;
	}
	else {
		printf("[libsrv_gpu_firewall] Info: packet_pool off\n");
		//system("killall packet_poll");

		off_flag = 0;
		break_flag=1;
		pthread_join(exec_thread, (void**)&r);
		printf("[libsrv_gpu_firewall] Info: packet_pool offed\n");
		nfq_destroy_queue(qh);
		nfq_close(h);
	}

	printf("[libsrv_gpu_firewall] Info: gpu_firewall_launch end\n");

	return 0;
}

int gpu_firewall_post(struct qhgpu_service_request *sr) {
	printf("[libsrv_gpu_firewall] Info: gpu_firewall_post\n");
	return 0;
}

static struct qhgpu_service gpu_firewall_srv;

unsigned long inet_aton_custom(const char * str) {
	unsigned long result = 0;
	unsigned int iaddr[4] = { 0, };
	unsigned char addr[4] = { 0, };

	int i;
	sscanf(str, "%d.%d.%d.%d ", iaddr, iaddr + 1, iaddr + 2, iaddr + 3);
	for (i = 0; i < 4; i++) {
		addr[i] = (char) iaddr[i];
	}
	for (i = 3; i > 0; i--) {
		result |= addr[i];
		result <<= 8;
	}
	result |= addr[0];
	return result;
}

int do_work(int* packet_buff, int packet_buff_size) {
	return run_gpu_firewall(&(fw_data.context), &(fw_data.command_queue), &(fw_data.kernel), &(fw_data.mem_obj), (void*)packet_buff, packet_buff_size );
}

int init_service(void *lh, int (*reg_srv)(struct qhgpu_service*, void*), cl_context context, cl_device_id* device_id) {
	cl_int ret;
	struct timeval tv;

	printf("[libsrv_gpu_firewall] Info: init libsrv gpu firewall.\n");

	fw_data.context = context;
	fw_data.device_id = device_id;

	gettimeofday (&tv, NULL);
	perfo_rv_time = 0;
	first_rv_time = (tv.tv_sec * 1000 + tv.tv_usec / 1000);

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
