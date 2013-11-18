#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/ip.h>

#include <linux/syscalls.h>
#include <asm/uaccess.h>

#define IP_BASE (0x11111111)

static struct nf_hook_ops netfilter_ops;
unsigned long packet_id;
unsigned long perfo_time;

unsigned long inet_aton(const char * str);

void print_ip(int ip) {
	unsigned char bytes[4];
	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;
	printk("%3d.%3d.%3d.%3d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

unsigned int main_hook(unsigned int hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff*)) {

	int i;
	//char *data;
	struct iphdr *iph = ip_hdr(skb); // ip header
	unsigned long saddr = 0, daddr = 0; // ip address
	unsigned short source = 0, dest = 0; // port number
	//tcph, udph is apart from iph 20byte(size of(struct iphdr));
	struct tcphdr *tcph = (struct tcphdr*) ((char*) iph + sizeof(struct iphdr));
	struct udphdr *udph = (struct udphdr*) ((char*) iph + sizeof(struct iphdr));
	//unsigned short http = 80;

	//time val
	struct timeval t0, t1;
	long tt;

	//time stamp
	do_gettimeofday(&t0);

	//http port number : 80
	//get source and dest ip from iph

	saddr = iph->saddr;
	daddr = iph->daddr;
	//get source and dest port from tcph
	source = htons(tcph->source);
	dest = htons(tcph->dest);

	/*if (iph->protocol == IPPROTO_UDP) {
		printk("GFmod:UDP saddr[");
		print_ip(saddr);
		printk("]raddr[");
		print_ip(daddr);
		printk("]\n");
		printk("GFmod:UDP sport[%u]\tdport[%u]\n", source, dest);

		return NF_QUEUE;
	}
	else if(iph->protocol == IPPROTO_TCP){
		printk("GFmod:TCP saddr[");
		print_ip(saddr);
		printk("]raddr[");
		print_ip(daddr);
		printk("]\n");
		printk("GFmod:TCP sport[%u]\tdport[%u]\n", source, dest);

		return NF_QUEUE;
	}*/
	if( iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP ) {
		if( packet_id / 1000 > 0 ) {
			printk("[cpu firewall kernel module]perfo time: [%10lu]usec\n", perfo_time);
			perfo_time = 0;
			packet_id = 0;
		}
		packet_id++;

		/*if(saddr == 36203632 ) {
			do_gettimeofday(&t1);
			perfo_time += 1000000*(t1.tv_sec-t0.tv_sec) + ((long)(t1.tv_usec) - (long)(t0.tv_usec));

			return NF_DROP;
		}*/

		for( i = 0 ; i < 0x4000 ; i++ ) {
			if( saddr == IP_BASE + i ) {

				do_gettimeofday(&t1);
				perfo_time += 1000000*(t1.tv_sec-t0.tv_sec) + ((long)(t1.tv_usec) - (long)(t0.tv_usec));

				return NF_DROP;
			}
		}
	}

	do_gettimeofday(&t1);
	perfo_time += 1000000*(t1.tv_sec-t0.tv_sec) + ((long)(t1.tv_usec) - (long)(t0.tv_usec));

	return NF_ACCEPT;
}

int init_module(void) {
	printk("[cpu firewall kernel module]minit\n");

	netfilter_ops.hook = main_hook;
	netfilter_ops.pf = PF_INET;
	netfilter_ops.hooknum = NF_INET_PRE_ROUTING;
	netfilter_ops.priority = 1;
	nf_register_hook(&netfilter_ops);

	packet_id = 0;
	perfo_time = 0;

	return 0;
}

void cleanup_module(void) {
	nf_unregister_hook(&netfilter_ops);

	printk("[cpu firewall kernel module]mexit\n");
}

unsigned long inet_aton(const char * str) {
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

MODULE_LICENSE("GPL");
