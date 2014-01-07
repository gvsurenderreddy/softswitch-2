/*
 *	Forwarding decision
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/netfilter_bridge.h>
#include "br_private.h"

/* ram log */
#include <net/net_log.h>

extern int log_cnt;
extern char log_buf[MAX_LOG_CNT][MAX_LOG_LEN];
extern void update_log_cnt();

static struct timeval time_brf;
static struct timeval time_brfl;
/**********/

/* bridge port state correspondence
BR_STATE_DISABLED 0
BR_STATE_LISTENING 1
BR_STATE_LEARNING 2
BR_STATE_FORWARDING 3
BR_STATE_BLOCKING 4
*/

/* Don't forward packets to originating port or forwarding diasabled */
static inline int should_deliver(const struct net_bridge_port *p,
				 const struct sk_buff *skb)
{
	return (((p->flags & BR_HAIRPIN_MODE) || skb->dev != p->dev) &&
		p->state == BR_STATE_FORWARDING);
}

static inline unsigned packet_length(const struct sk_buff *skb)
{
	return skb->len - (skb->protocol == htons(ETH_P_8021Q) ? VLAN_HLEN : 0);
}

int br_dev_queue_push_xmit(struct sk_buff *skb)
{
	/* drop mtu oversized packets except gso */
	if (packet_length(skb) > skb->dev->mtu && !skb_is_gso(skb))
		kfree_skb(skb);
	else {
		/* ip_refrag calls ip_fragment, doesn't copy the MAC header. */
		if (nf_bridge_maybe_copy_header(skb))
			kfree_skb(skb);
		else {
			skb_push(skb, ETH_HLEN);

			dev_queue_xmit(skb);
		}
	}

	return 0;
}

int br_forward_finish(struct sk_buff *skb)
{
	return NF_HOOK(PF_BRIDGE, NF_BR_POST_ROUTING, skb, NULL, skb->dev,
		       br_dev_queue_push_xmit);

}

static void __br_deliver(const struct net_bridge_port *to, struct sk_buff *skb)
{
        /* ram log */
        do_gettimeofday(&time_brf);
        to->dev->stats.rx_bytes += skb->len;
        to->dev->stats.rx_packets++;
        sprintf(log_buf[log_cnt], "BRIDGE_DELIVER(%ld.%06ld): %s(%d) via %s(%d) to %s(%d), %ld %ld %ld %ld, %ld %ld %ld %ld, %ld %ld %ld %ld, %u %d\n", time_brf.tv_sec, time_brf.tv_usec, 
               skb->dev->name, skb->dev->mtu, 
               to->br->dev->name, to->br->dev->mtu,
               to->dev->name, to->dev->mtu,
               skb->dev->stats.tx_bytes, skb->dev->stats.tx_packets, skb->dev->stats.rx_bytes, skb->dev->stats.rx_packets,
               to->br->dev->stats.tx_bytes, to->br->dev->stats.tx_packets, to->br->dev->stats.rx_bytes, to->br->dev->stats.rx_packets,      
               to->dev->stats.tx_bytes, to->dev->stats.tx_packets, to->dev->stats.rx_bytes, to->dev->stats.rx_packets,
               skb->len, skb_shinfo(skb)->nr_frags);
        update_log_cnt();
        /**********/

	skb->dev = to->dev;
	NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_OUT, skb, NULL, skb->dev,
			br_forward_finish);
}

static void __br_forward(const struct net_bridge_port *to, struct sk_buff *skb)
{
	struct net_device *indev;

	if (skb_warn_if_lro(skb)) {
		kfree_skb(skb);
		return;
	}

	indev = skb->dev;
	skb->dev = to->dev;
	skb_forward_csum(skb);

        /* ram log */
        do_gettimeofday(&time_brf);
        to->dev->stats.rx_bytes += skb->len;
        to->dev->stats.rx_packets++;
        sprintf(log_buf[log_cnt], "BRIDGE_FWD(%ld.%06ld): %s(%d) via %s(%d) to %s(%d), %ld %ld %ld %ld, %ld %ld %ld %ld, %ld %ld %ld %ld, %u %d\n", time_brf.tv_sec, time_brf.tv_usec, 
                indev->name, indev->mtu,
                to->br->dev->name, to->br->dev->mtu,
                to->dev->name, to->dev->mtu,
                indev->stats.tx_bytes, indev->stats.tx_packets, indev->stats.rx_bytes, indev->stats.rx_packets,
                to->br->dev->stats.tx_bytes, to->br->dev->stats.tx_packets, to->br->dev->stats.rx_bytes, to->br->dev->stats.rx_packets,
                to->dev->stats.tx_bytes, to->dev->stats.tx_packets, to->dev->stats.rx_bytes, to->dev->stats.rx_packets,
                skb->len, skb_shinfo(skb)->nr_frags);
        update_log_cnt();
        /**********/

	NF_HOOK(PF_BRIDGE, NF_BR_FORWARD, skb, indev, skb->dev,
			br_forward_finish);
}

/* called with rcu_read_lock */
void br_deliver(const struct net_bridge_port *to, struct sk_buff *skb)
{
	if (should_deliver(to, skb)) {
		__br_deliver(to, skb);
		return;
	}

	kfree_skb(skb);
}

/* called with rcu_read_lock */
void br_forward(const struct net_bridge_port *to, struct sk_buff *skb)
{
	if (should_deliver(to, skb)) {
		__br_forward(to, skb);
		return;
	}

	kfree_skb(skb);
}

/* called under bridge lock */
static void br_flood(struct net_bridge *br, struct sk_buff *skb,
	void (*__packet_hook)(const struct net_bridge_port *p,
			      struct sk_buff *skb))
{
	struct net_bridge_port *p;
	struct net_bridge_port *prev;

	prev = NULL;

        /* ram log */
        do_gettimeofday(&time_brfl);
        /**********/

	list_for_each_entry_rcu(p, &br->port_list, list) {

		if (should_deliver(p, skb)) {
			if (prev != NULL) {
				struct sk_buff *skb2;

				if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
					br->dev->stats.tx_dropped++;
					kfree_skb(skb);
					return;
				}

                                /* ram log */
                                p->dev->stats.rx_bytes += skb->len;
                                p->dev->stats.rx_packets++;
                                sprintf(log_buf[log_cnt], "BRIDGE_FLOOD(%ld.%06ld): %s(%d) via %s(%d) to %s(%d), %ld %ld %ld %ld, %ld %ld %ld %ld, %ld %ld %ld %ld, %u %d\n", time_brfl.tv_sec, time_brfl.tv_usec, 
                                         skb->dev->name, skb->dev->mtu,
                                         br->dev->name, br->dev->mtu,
                                         p->dev->name, p->dev->mtu,
                                         skb->dev->stats.tx_bytes, skb->dev->stats.tx_packets, skb->dev->stats.rx_bytes, skb->dev->stats.rx_packets,
                                         br->dev->stats.tx_bytes, br->dev->stats.tx_packets, br->dev->stats.rx_bytes, br->dev->stats.rx_packets,
                                         p->dev->stats.tx_bytes, p->dev->stats.tx_packets, p->dev->stats.rx_bytes, p->dev->stats.rx_packets,
                                         skb->len, skb_shinfo(skb)->nr_frags);     
                                 update_log_cnt();
                                /**********/

				__packet_hook(prev, skb2);
			}

			prev = p;
		}
	}

	if (prev != NULL) {
		__packet_hook(prev, skb);
		return;
	}

	kfree_skb(skb);
}


/* called with rcu_read_lock */
void br_flood_deliver(struct net_bridge *br, struct sk_buff *skb)
{
	br_flood(br, skb, __br_deliver);
}

/* called under bridge lock */
void br_flood_forward(struct net_bridge *br, struct sk_buff *skb)
{
	br_flood(br, skb, __br_forward);
}
