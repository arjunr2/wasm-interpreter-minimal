/***********************************************************************
* File: packetCaptureWithNetfilter.c
* Copy Right: http://www.cnblogs.com/piky/articles/1587767.html
* Author: Unknow
* Create Date: Unknow
* Abstract Description:
*             To catch the packet from the network with netfilter.
*
*------------------------Revision History------------------------
* No.    Date        Revised By   Description
* 1      2011/7/28   Sam          +print the payload.(unsuccessful)
* 2      2011/7/30   Sam          +get the packet from the specific ip.
* 3      2011/8/10   Sam          correct the checksum and
*                                 +print the payload.
***********************************************************************/





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
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/netfilter_ipv4.h>

#define SIP     "192.168.1.55"

static struct nf_hook_ops nfho;

static int counter = 0;

unsigned int hook_func(void *priv,
                       struct sk_buff *skb,
                       const struct nf_hook_state *state)
{
			if(skb) {
                struct udphdr *udph = NULL;
                struct iphdr *iph = NULL;
                u8 *payload;    // The pointer for the tcp payload.
                //char sourceAddr[20];
                //char myAddr[20];

                iph = ip_hdr(skb);
                if(iph) {
												__be32 myAddr = in_aton(SIP);
												__be32 sourceAddr = iph->saddr;
												//pr_err("S: %pI4 ; M: %pI4\n", &sourceAddr, &myAddr);
                        //sprintf(myAddr, SIP);
                        //sprintf(sourceAddr, "%pI4", &iph->saddr);

                        if (myAddr == sourceAddr) {
                                pr_err("IP:[%pI4]-->[%pI4];\n",
                                        &iph->saddr, &iph->daddr);
                                pr_err("IP (version %u, ihl %u, tos 0x%x, ttl %u, id %u, length %u, ",
                                        iph->version, iph->ihl, iph->tos, iph->ttl,
                                        ntohs(iph->id), ntohs(iph->tot_len));
                                switch (iph->protocol) {
																	case IPPROTO_UDP:
																					/*get the udp information*/
																					udph = (struct udphdr *)(skb->data + iph->ihl*4);
																					printk("UDP: [%u]-->[%u];\n", ntohs(udph->source), ntohs(udph->dest));    
																					payload = (char *)udph + (char)sizeof(struct udphdr);
																					printk("Payload: { \n%s\n }", payload);
																					counter++;
																					break;
																	default:
																					printk("unknown protocol!\n");
																					break;
                                }
                        }
                } else
                        pr_err("iph is null\n");
        } else
                pr_err("skb is null\n");

        return NF_ACCEPT;         
}

int init_module()
{
        nfho.hook = hook_func;        
        nfho.hooknum  = NF_INET_PRE_ROUTING;
        nfho.pf = AF_INET;
        nfho.priority = NF_IP_PRI_FIRST;

        nf_register_net_hook(&init_net, &nfho);

        printk("init module----------------ok\n");

        return 0;
}

void cleanup_module()
{
        nf_unregister_net_hook(&init_net, &nfho);
        printk("exit module----------------ok\n");
				printk("Num packets: %d\n", counter);
}

MODULE_LICENSE("GPL");
