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

#include "myprotocol.h"
#define DROP_NAME "drop"
#define DROP_MAJOR 240

unsigned long filter_addr[10];

/* array that contains ip addresses to drop */
unsigned int filter_addr_cnt = 0;
static struct nf_hook_ops netfilter_ops;
struct sk_buff *sock_buff;
struct file_operations drop_fops;

void add_addr(const char*);

/* open() system call */
int drop_open(struct inode *inode, struct file *filp) {
	return 0;
}

/* close() system call */
int drop_release(struct inode *inode, struct file *filp) {
	return 0;
}

/* write() system call */
ssize_t drop_write(struct file *filp, const char *buf, size_t count,
		loff_t *fpos) {
	struct myprotocol* pmsg = (struct myprotocol*) buf;
	if (pmsg->cmd == ADD_IP) {
		char *addr = pmsg->addr;
		add_addr(addr);
	}
	return 0;

}

void print_ip(int ip) {
	unsigned char bytes[4];
	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;
	printk("%3d.%3d.%3d.%3d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

unsigned long inet_aton(const char*);

unsigned int main_hook(unsigned int hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff*)) {

	int i;
	char *data;
	struct iphdr *iph = ip_hdr(skb); /* ip header*/
	unsigned long saddr = 0, daddr = 0; /* ip address */
	unsigned short source = 0, dest = 0; /* port number */
	//tcph, udph is apart from iph 20byte(size of(struct iphdr));
	struct tcphdr *tcph = (struct tcphdr*) ((char*) iph + sizeof(struct iphdr));
	struct udphdr *udph = (struct udphdr*) ((char*) iph + sizeof(struct iphdr));
	unsigned short http = 80;

	//time val
	struct timeval t0, t1;
	long tt;

	//time stamp
	//do_gettimeofday(&t0);

	//http port number : 80
	//get source and dest ip from iph
	saddr = iph->saddr;
	daddr = iph->daddr;
	//get source and dest port from tcph
	source = htons(tcph->source);
	dest = htons(tcph->dest);

	if (iph->protocol == IPPROTO_UDP) {
		data = (char*) udph + sizeof(*udph);
		/* print udp data */
		//printk("<1>source : %u \t dest : %u\n %s\n", source, dest, data);

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
	}

	return NF_ACCEPT;

	//do_gettimeofday(&t1);
	//tt = 1000000*(t1.tv_sec-t0.tv_sec) + ((long)(t1.tv_usec) - (long)(t0.tv_usec));
	//printk("main hook time: %10lu MS\n", tt);

	/*for (i = 0; i < filter_addr_cnt; i++) {
		if (saddr == filter_addr[i]) {
			// ip drop
			return NF_DROP;
		}
	}
	if (source == http) {
		return NF_DROP;
	}*/
	//http port Drop
	//return NF_ACCEPT;

	//todo : buff count 가 차거나 일정 시간 이상 경과 시
	//qhgpu sync or async call 호출

}

int init_module() {
	drop_fops.owner = THIS_MODULE;
	drop_fops.open = drop_open;
	drop_fops.release = drop_release;
	drop_fops.write = drop_write;
	netfilter_ops.hook = main_hook;
	netfilter_ops.pf = PF_INET;
	netfilter_ops.hooknum = NF_INET_PRE_ROUTING;
	netfilter_ops.priority = 1;
	nf_register_hook(&netfilter_ops);
	//모듈등록
	register_chrdev(DROP_MAJOR, DROP_NAME, &drop_fops);
	return 0;
}

void cleanup_module() {
	nf_unregister_hook(&netfilter_ops);
	//모듈해제
	unregister_chrdev(DROP_MAJOR, DROP_NAME);
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

/* add address to drop */
void add_addr(const char *addr) {
	filter_addr[filter_addr_cnt++] = inet_aton(addr);
	printk("<1> %s(%lu) drop added! \n", addr, filter_addr[filter_addr_cnt]);
}

