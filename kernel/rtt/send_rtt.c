
#ifndef __KERNEL__
    #define __KERNEL__
#endif
#ifndef MODULE
    #define MODULE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/inet.h>
#include <net/ip.h>
#include <net/udp.h>
#include <linux/netfilter_ipv4.h>
#include <linux/delay.h>
#include <linux/timekeeping.h>
#include <net/sock.h>
#include <linux/udp.h>
#include <linux/net.h>


char *server_ip="192.168.1.216";
unsigned short port=8008;
struct sockaddr_in sendaddr;
struct socket *sock;
char sendstring[] = "hello world";

// Send globals
static struct task_struct *send_kthread;

// Receive globals
char *client_ip = "192.168.1.155";
struct sockaddr_in recvaddr;


static struct nf_hook_ops nfho;
static int counter = 0;
static volatile int receive_flag = 0;
static ktime_t recv_msg_time;


unsigned int hook_func(void *priv, struct sk_buff *skb, 
				const struct nf_hook_state *state) {
	if(skb) {
		struct udphdr *udph = NULL;
		struct iphdr *iph = NULL;
		u8 *payload;    // The pointer for the tcp payload.

		iph = ip_hdr(skb);
		if(iph) {
			__be32 serverAddr = in_aton(server_ip);
			__be32 sourceAddr = iph->saddr;

			if (serverAddr == sourceAddr) {
				switch (iph->protocol) {
					case IPPROTO_UDP:
						/*get the udp information*/
						udph = (struct udphdr *)(skb->data + iph->ihl*4);
						payload = (char*)udph + (char)sizeof(struct udphdr);
						counter++;
						//pr_err("Received payload { %s }\n", payload);
						recv_msg_time = ktime_get();
						receive_flag = 1;
						break;
					default:
						printk("unknown protocol!\n");
						break;
				}
			}
		} else {
				pr_err("iph is null\n");
		}
	} else {
			pr_err("skb is null\n");
	}
	return NF_ACCEPT;         
}

static int send_msg(struct socket *sock, char *buffer, size_t length) {
	struct msghdr        msg;
	struct kvec        iov = {0};
	int                len;

	// Construct target address
	memset(&sendaddr,0,sizeof(sendaddr));
	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(port);
	sendaddr.sin_addr.s_addr = in_aton(server_ip);
 
	memset(&msg,0,sizeof(msg));
	msg.msg_flags = MSG_DONTWAIT|MSG_NOSIGNAL;       
	msg.msg_name = (struct sockaddr *)&sendaddr;
	msg.msg_namelen = sizeof(sendaddr);

	iov.iov_base     = (void *)buffer;
	iov.iov_len      = length;

	len = kernel_sendmsg(sock, &msg, &iov, 1, length);
	return len;
}



static void make_client_socket(void) {

	memset(&recvaddr, 0, sizeof(recvaddr));
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(port);
	recvaddr.sin_addr.s_addr = in_aton(client_ip);

	int err;
	if ( (err = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock)) < 0){
		printk(KERN_ALERT "sock_create_kern error: %d \n", err);
		goto error;
	}
	if ( (err = kernel_bind(sock, (struct sockaddr*)&recvaddr, sizeof(struct sockaddr))) < 0){
		printk(KERN_ALERT "sock bind error: %d \n", err);
		goto error;
	}
	return;
error:
	sock = NULL;
	return;
}


int send_rtt_function(void *args) {

	make_client_socket();
	if(sock == NULL)
		return -1;

	// Receiver hook registration
	nfho.hook = hook_func;        
	nfho.hooknum  = NF_INET_PRE_ROUTING;
	nfho.pf = AF_INET;
	nfho.priority = NF_IP_PRI_FIRST;
	nf_register_net_hook(&init_net, &nfho);

	printk("Starting send RTT\n");
	char sendstring[] = "hello world";

	for (int i = 0; i < 3000; i++) {
		printk("Sending msg %d\n", i);
		// Timstamp and send message
		ktime_t start_time = ktime_get();
		send_msg(sock,sendstring,strlen(sendstring));
		while ( ktime_ms_delta(ktime_get(), start_time) < 1000 ) {
			if (receive_flag == 1) { break; }
		}
		if (receive_flag) {
			pr_err("RTT Time:%lld\n", ktime_us_delta(recv_msg_time, start_time));
		} else {
			pr_err("RTT Time:0\n");
		}
		receive_flag = 0;
		// Timestamp
		udelay(5000);
	}
	printk("Finished send RTT\n");
	do_exit(0);
}


static int __init send_rtt_init(void) {
	// Send thread
	printk("Initializing SendRTT Thread\n");
	send_kthread = kthread_run(send_rtt_function, NULL, "send_rtt_function");
	if (send_kthread) {
		printk("SendRTT thread initialized run!\n");
	} else {
		pr_err("SendRTT thread could not be created!\n");
		return -1;
	}
	return 0;
}

static void __exit send_rtt_exit(void) {
	printk("Tearing down sender RTT\n");
	if(sock) {
		sock_release(sock);
		nf_unregister_net_hook(&init_net, &nfho);
	}
	printk("Torn down sender RTT\n");
	return;
}

module_init(send_rtt_init);
module_exit(send_rtt_exit);

MODULE_LICENSE("GPL");
