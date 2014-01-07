#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

#include "ram_bridge_log.h"

int bridge_print_flush = 0;
module_param(bridge_print_flush, int, 0000);
MODULE_PARM_DESC(bridge_print_flush, "0 to print and 1 to flush");

mm_segment_t bridge_oldfs;

extern int bridge_log_cnt;
extern char bridge_log_buf[MAX_LOG_CNT][MAX_LOG_LEN]; //100000000B < 100MB
extern void bridge_flush_log();

struct file *bridge_openFile(char *path, int flag, int mode)
{
   struct file *fp;
   fp = filp_open(path, flag, 0);
   if (fp)
      return fp;
   else
      return NULL;
}

int bridge_writeFile(struct file *fp, char *buf, int writelen, int writepos)
{
   if (fp->f_op && fp->f_op->write)
   {
       fp->f_pos = writepos;
       return fp->f_op->write(fp, buf, writelen, &fp->f_pos);
   }
   else
   {
       return -1;
   }
}

int bridge_closeFile(struct file *fp)
{
   filp_close(fp, NULL);
   set_fs(bridge_oldfs);
   return 0;
}

void bridge_initKernelEnv(void)
{
   bridge_oldfs = get_fs();
   set_fs(KERNEL_DS);
}

void bridge_fprint_log(struct file *fp)
{
   int i;
   long byte_write = 0;
   for (i = 0; i < bridge_log_cnt; i++)
   {
       if (bridge_writeFile(fp, bridge_log_buf[i], strlen(bridge_log_buf[i]), byte_write) < 0)
       {
           printk("BRIDGE: ram log print fail at line %d\n", i+1);
           break;
       }
       byte_write += strlen(bridge_log_buf[i]);
   }
}

static int __init bridge_ram_log_init(void)
{
    struct file *fp;
    if (bridge_print_flush == 0)
    {
        /* init ram log file */
        bridge_initKernelEnv();
        fp = bridge_openFile("/etc/bridge_myconfig", O_RDWR|O_CREAT, 0);
        if (fp != NULL)
            printk("BRIDGE: ram log file created\n");
        else
            printk("BRIDGE: ram log file fail\n");
        
        bridge_fprint_log(fp);
        bridge_closeFile(fp);
    }
    bridge_flush_log();    
    return 0;
}

static void __exit bridge_ram_log_exit(void)
{
    printk("BRIDGE: ram_log_file module remove successfully\n");
}

module_init(bridge_ram_log_init);
module_exit(bridge_ram_log_exit);

MODULE_DESCRIPTION("ram log in drivers/xen/bridge module"); 
MODULE_AUTHOR("Ke Hong<khongaa@ust.hk>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("bridge ram log module");

