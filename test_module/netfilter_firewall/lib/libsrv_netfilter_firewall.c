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
//#include <openssl/md5.h>

#define IFNAMSIZ 16

#define BUFSIZE 2048
#define PAYLOADSIZE 2048

#define SRC_ADDR(payload) (*(in_addr_t *)((payload)+12))
#define DST_ADDR(payload) (*(in_addr_t *)((payload)+16))

#define EXTRACT_32BITS(p) \
          ((u_int32_t)ntohl(((const unaligned_u_int32_t *)(p))))

#define TCP 6
#define UDP 17

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

int show_packet(unsigned char *dgram, unsigned int datalen)
{
        struct iphdr *iphdrs;
        struct tcphdr *tcphdrs;
        struct udphdr *udphdrs;
        struct in_addr addr;
        uint32_t seq, ack;

        char md5msg[40];
        char source[64];
        int len;

        char *show_data;

        iphdrs = (struct iphdr *)dgram;

        switch(iphdrs->protocol)
        {
                case TCP:
/*
                        tcphdrs = (struct tcphdr *)(dgram + 4 * iphdrs->ihl);
                        show_data = (char *)(dgram + datalen - 40);
                        printf("%s\n", show_data);
*/
                        break;

                case UDP:
                        udphdrs = (struct udphdr *)(dgram + sizeof(struct iphdr));
/*
                        printf("sport : %u \t dport : %u\n", htons(udphdrs->source), htons(udphdrs->dest));
                        printf("udp size : %u\n", htons(udphdrs->len));
*/
                        show_data = (char *)(dgram + datalen - sizeof(struct iphdr) - sizeof(struct udphdr));

//                      printf("%s\n", show_data);
                        if(strstr(show_data, ",,,,,,,,,,"))
                                return NF_DROP;
                        break;

                default:
                        printf("protocol num %d is unknown protocol\n", iphdrs->protocol);
                        break;
        }

        return NF_ACCEPT;
}

static int work_do(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
        int id = 0, status = 0;
        struct queued_pkt q_pkt;
        struct nfqnl_msg_packet_hdr *ph;
        struct iphdr *iph = NULL;
        struct timeval timestamp;
        unsigned char *payload;
        int ret;
        u_int32_t POLICY;

#ifdef HAVE_NFQ_INDEV_NAME
  struct nlif_handle *nlif_handle = (struct nlif_handle *) data;
#endif

	q_pkt.payload_len = nfq_get_payload(nfa, &(q_pkt.payload));
	q_pkt.mark = nfq_get_nfmark(nfa);

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

        ret = nfq_get_timestamp(nfa, &timestamp);
        if(ret == 0)
                q_pkt.timestamp = timestamp.tv_sec;
        else
                q_pkt.timestamp = time(NULL);

        ph = nfq_get_msg_packet_hdr(nfa);
        if(ph) {
                id = ntohl(ph->packet_id);
                nfq_get_payload(nfa, &payload);

                switch(ph->hook)
                {
                        case NF_IP_LOCAL_IN:
                                POLICY = show_packet(q_pkt.payload, q_pkt.payload_len);
                                break;

                        case NF_IP_LOCAL_OUT:
                                POLICY = show_packet(q_pkt.payload, q_pkt.payload_len);
                                break;

                        case NF_IP_FORWARD:
                                POLICY = show_packet(q_pkt.payload, q_pkt.payload_len);
                                break;
                }

                nfq_set_verdict(qh, id, POLICY, 0, NULL);
        }
}

int netlink_init(int q_num)
{
        struct nfq_handle *h;
        struct nfq_q_handle *qh;
        struct nfnl_handle *nh;
        int fd, rv;
        char buf[BUFSIZE];

        h = nfq_open();
        if(!h) {
                fprintf(stderr, "Error During nfq_open()\n");
                exit(-1);
        }

        if(nfq_unbind_pf(h, AF_INET) < 0) {
                fprintf(stderr, "Error During nfq_unbind_pf()\n");
        }

        if(nfq_bind_pf(h, AF_INET) < 0) {
                fprintf(stderr, "Error During nfq_bind_pf()\n");
                exit(-1);
        }

        qh = nfq_create_queue(h, q_num, &work_do, NULL);
        if(!qh) {
                fprintf(stderr, "Error During nfq_create_queue()\n");
                exit(-1);
        }

        if(nfq_set_mode(qh, NFQNL_COPY_PACKET, PAYLOADSIZE) < 0) {
                fprintf(stderr, "can't set packet_copy mode\n");
                exit(-1);
        }

        fprintf(stderr, "NFQUEUE:binding from queue\n");

        nh = nfq_nfnlh(h);
        fd = nfnl_fd(nh);

        while((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0)
        {
                nfq_handle_packet(h, buf, rv);
        }

        fprintf(stderr, "NFQUEUE:unbinding from queue\n");
        nfq_destroy_queue(qh);
        nfq_close(h);
        return(0);
}

int main(int argc, char **argv)
{
        int q_num = 0;
        if(argc < 2) {
                fprintf(stderr, "Option error\nNF Queue Num default setting 0.\n");
                q_num = 0;
        }
        else
                q_num = atoi(argv[1]);
        return netlink_init(q_num);
}
