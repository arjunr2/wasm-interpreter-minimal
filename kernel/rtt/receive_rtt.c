
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
#include <linux/net.h>


char *destination_ip="192.168.1.76";
char sendstring[] = "hello world pong";
unsigned short dport=8008;
struct sockaddr_in recvaddr;
struct socket *sock;

// Pong globals
static struct task_struct *pong_kthread;

// Receive globals
static struct nf_hook_ops nfho;
static int counter = 0;
static volatile int receive_buf = 0;

static int send_msg(struct socket *sock, char *buffer, size_t length) {
	struct msghdr        msg;
	struct kvec        iov = {0};
	int                len;
 
	memset(&msg,0,sizeof(msg));
	msg.msg_flags = MSG_DONTWAIT|MSG_NOSIGNAL;       
	msg.msg_name = (struct sockaddr *)&recvaddr;
	msg.msg_namelen = sizeof(recvaddr);


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
			__be32 myAddr = in_aton(destination_ip);
			__be32 sourceAddr = iph->saddr;
			//pr_err("S: %pI4 ; M: %pI4\n", &sourceAddr, &myAddr);

			if (myAddr == sourceAddr) {
				//pr_err("IP:[%pI4]-->[%pI4];\n", &iph->saddr, &iph->daddr);
				switch (iph->protocol) {
					case IPPROTO_UDP:
						/*get the udp information*/
						udph = (struct udphdr *)(skb->data + iph->ihl*4);
						payload = (char *)udph + (char)sizeof(struct udphdr);
						printk("Payload: { \n%s\n }", payload);
						receive_buf++;
						counter++;
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


static void make_daddr(void) {
	memset(&recvaddr,0,sizeof(recvaddr));
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(dport);
	recvaddr.sin_addr.s_addr = in_aton(destination_ip);
}


static void make_socket(void) {
	if(sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock) < 0){
		printk(KERN_ALERT "sock_create_kern error\n");
		sock = NULL;
		return;
	}
	if(sock->ops->connect(sock, (struct sockaddr*)&recvaddr,
														 sizeof(struct sockaddr), 0) < 0){
		printk(KERN_ALERT "sock connect error\n");
		goto error;
	}
	return;
error:
	sock_release(sock);
	sock = NULL;
	return;
}


int pong_rtt_function(void* args) {
	while (!kthread_should_stop()) {
		if (receive_buf > 0) {
			send_msg(sock, sendstring, strlen(sendstring));
			printk("Sent message\n");
			receive_buf--;
		}
	}
	return 0;
}


static int __init pong_rtt_init(void) {
	make_daddr();
	make_socket();
	if(sock == NULL)
		return -1;


	pong_kthread = kthread_run(pong_rtt_function, NULL, "pong_rtt_function");
	if (pong_kthread) {
		printk("PongRTT thread initialized run!\n");
	} else {
		pr_err("PongRTT thread could not be created!\n");
		return -1;
	}

	// Receiver hook registration
	nfho.hook = hook_func;        
	nfho.hooknum  = NF_INET_PRE_ROUTING;
	nfho.pf = AF_INET;
	nfho.priority = NF_IP_PRI_FIRST;
	nf_register_net_hook(&init_net, &nfho);

	printk("Starting receiving RTT\n");
	
	return 0;
}

static void __exit pong_rtt_exit(void) {
	if(sock)
		sock_release(sock);
  nf_unregister_net_hook(&init_net, &nfho);
	printk("Tear down pong RTT\n");
	kthread_stop(pong_kthread);
	printk("Num packets: %d\n", counter);
	return;
}

module_init(pong_rtt_init);
module_exit(pong_rtt_exit);

MODULE_LICENSE("GPL");
