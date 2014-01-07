#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/vmalloc.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <asm/uaccess.h>

#include "br_private.h"

int new_mtu_size = 0;
module_param(new_mtu_size, int, 0000);
MODULE_PARM_DESC(new_mtu_size, "size of new mtu");

extern struct net_bridge *xen_net_bridge;

extern int br_min_mtu(const struct net_bridge *br);

int ori_mtu_size = 1500;

static int br_set_mtu(int new_mtu)
{
   struct net_bridge_port *p;

   if (list_empty(&xen_net_bridge->port_list) || !new_mtu || new_mtu < 68)
        return -1;

   else {
        list_for_each_entry(p, &xen_net_bridge->port_list, list) {
            p->dev->mtu = new_mtu;
        }

#ifdef CONFIG_BRIDGE_NETFILTER
        /* remember the MTU in the rtable for PMTU */
        xen_net_bridge->fake_rtable.u.dst.metrics[RTAX_MTU - 1] = new_mtu;
#endif

       xen_net_bridge->dev->mtu = new_mtu;
  }
   
  return 0;
}

static int __init br_new_mtu_init(void)
{
    //ori_mtu_size = br_min_mtu(xen_net_bridge);
 
    ori_mtu_size = xen_net_bridge->dev->mtu > ori_mtu_size? ori_mtu_size : xen_net_bridge->dev->mtu;
    printk("br_new_mtu module changing mtu from %d to %d\n", ori_mtu_size, new_mtu_size);

    if (br_set_mtu(new_mtu_size) == 0) 
        printk("br_new_mtu module install successfully, %d\n", new_mtu_size); 
    else
        printk("br_new_mtu module install fail, %d\n", ori_mtu_size); 

   return 0;
}

static void __exit br_new_mtu_exit(void)
{
    br_set_mtu(ori_mtu_size);
    printk("br_new_mtu module remove successfully, %d\n", ori_mtu_size);
}

module_init(br_new_mtu_init);
module_exit(br_new_mtu_exit);

MODULE_DESCRIPTION("br_new_mtu in kernel module");
MODULE_AUTHOR("Ke Hong<khongaa@ust.hk>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("br_new_mtu module");
