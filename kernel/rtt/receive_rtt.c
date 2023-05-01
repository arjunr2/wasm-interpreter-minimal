
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
#include <net/sock.h>
#include <linux/udp.h>
#include <linux/socket.h>
#include <linux/net.h>

// Server settings
char *server_ip="192.168.1.216";
unsigned short port=8008;
struct sockaddr_in recvaddr;
struct socket *sock;

char *client_ip="192.168.1.109";

// Pong globals
static struct task_struct *pong_kthread;
char sendstring[] = "hello world pong";

// Receive globals
static struct nf_hook_ops nfho;
static int counter = 0;
static volatile int receive_buf = 0;

static int send_response_msg(struct socket *sock, char *buffer, size_t length) {
	struct msghdr        msg;
	struct kvec        iov = {0};
	int                len;

	// Construct target address
	struct sockaddr_in client_addr;
	memset(&client_addr,0,sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);
	client_addr.sin_addr.s_addr = in_aton(client_ip);

	memset(&msg,0,sizeof(msg));
	msg.msg_flags = MSG_DONTWAIT|MSG_NOSIGNAL;       
	msg.msg_name = (struct sockaddr *)&client_addr;
	msg.msg_namelen = sizeof(client_addr);

	iov.iov_base     = (void *)buffer;
	iov.iov_len      = length;

	len = kernel_sendmsg(sock, &msg, &iov, 1, length);
	return len;
}


unsigned int hook_func(void *priv, struct sk_buff *skb, 
				const struct nf_hook_state *state) {
	if(skb) {
		struct udphdr *udph = NULL;
		struct iphdr *iph = NULL;
		u8 *payload;    // The pointer for the tcp payload.

		iph = ip_hdr(skb);
		if(iph) {
			__be32 clientAddr = in_aton(client_ip);
			__be32 sourceAddr = iph->saddr;

			if (clientAddr == sourceAddr) {
				//printk("IP:[%pI4]-->[%pI4];\n", &iph->saddr, &iph->daddr);
				switch (iph->protocol) {
					case IPPROTO_UDP:
						/*get the udp information*/
						udph = (struct udphdr *)(skb->data + iph->ihl*4);
						payload = (char *)udph + (char)sizeof(struct udphdr);
						receive_buf++;
						counter++;
						break;
					default:
						pr_err("unknown protocol!\n");
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


static void make_server_socket(void) {
	memset(&recvaddr,0,sizeof(recvaddr));
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(port);
	recvaddr.sin_addr.s_addr = in_aton(server_ip);

	if(sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock) < 0){
		printk(KERN_ALERT "sock_create_kern error\n");
		goto error;
	}
	int err;
	if ((err = kernel_bind (sock, (struct sockaddr*)&recvaddr, sizeof(struct sockaddr) )) < 0) {
		printk(KERN_ALERT "sock bind error: %d\n", err);
		goto error;
	}
	return;
error:
	sock = NULL;
	return;
}


int pong_rtt_function(void* args) {
	while (!kthread_should_stop()) {
		if (receive_buf > 0) {
			send_response_msg(sock, sendstring, strlen(sendstring) + 1);
			receive_buf--;
		}
	}
	return 0;
}


static int __init pong_rtt_init(void) {
	make_server_socket();
	if(sock == NULL)
		return -1;

	// Receiver hook registration
	nfho.hook = hook_func;        
	nfho.hooknum  = NF_INET_PRE_ROUTING;
	nfho.pf = AF_INET;
	nfho.priority = NF_IP_PRI_FIRST;
	nf_register_net_hook(&init_net, &nfho);

	pong_kthread = kthread_run(pong_rtt_function, NULL, "pong_rtt_function");
	if (pong_kthread) {
		printk("PongRTT thread initialized run!\n");
	} else {
		pr_err("PongRTT thread could not be created!\n");
		return -1;
	}

	printk("Starting receiving RTT\n");
	
	return 0;
}

static void __exit pong_rtt_exit(void) {
	printk("Num packets: %d\n", counter);
	if(sock)
		sock_release(sock);
  nf_unregister_net_hook(&init_net, &nfho);
	kthread_stop(pong_kthread);
	printk("Torn down PongRTT\n");
	return;
}

module_init(pong_rtt_init);
module_exit(pong_rtt_exit);

MODULE_LICENSE("GPL");
