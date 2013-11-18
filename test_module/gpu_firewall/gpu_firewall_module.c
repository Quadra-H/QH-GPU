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

#include "../../qhgpu/qhgpu.h"

static struct nf_hook_ops netfilter_ops;

struct qhgpu_request *req;

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

	struct iphdr *iph = ip_hdr(skb); // ip header

	if( iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP ) {
		return NF_QUEUE;
	}

	return NF_ACCEPT;
}

static int gpu_firewall_module_callback(struct qhgpu_request *req) {
	//req->out have return data
	struct completion *c;

	printk("[gpu firewall kernel module]callback start.\n");

	c = (struct completion*)req->kdata;
	complete(c);

	return 0;
}

int init_module(void) {
	printk("[gpu firewall kernel module]minit\n");

	netfilter_ops.hook = main_hook;
	netfilter_ops.pf = PF_INET;
	netfilter_ops.hooknum = NF_INET_PRE_ROUTING;
	netfilter_ops.priority = 1;
	nf_register_hook(&netfilter_ops);

	req = qhgpu_alloc_request( 0, "gpu_firewall_service");
	if (!req) {
		printk("request null\n");
		return 0;
	}

	req->callback = gpu_firewall_module_callback;

	qhgpu_call_sync(req);

	return 0;
}

void cleanup_module(void) {
	qhgpu_call_sync(req);

	nf_unregister_hook(&netfilter_ops);

	printk("[gpu firewall kernel module]mexit\n");
}

MODULE_LICENSE("GPL");
