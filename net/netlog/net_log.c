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

#include <net/net_log.h>

char log_buf[MAX_LOG_CNT][MAX_LOG_LEN]; //100000000B < 100MB
int log_cnt = 0;

EXPORT_SYMBOL(log_cnt);
EXPORT_SYMBOL(log_buf);

void flush_log()
{
   int i;
   for (i = 0; i < MAX_LOG_CNT; i++)
        memset(log_buf[i], 0, MAX_LOG_LEN);
   log_cnt = 0;
   printk("CUBIC: ram log flush\n");
}

EXPORT_SYMBOL(flush_log);

inline void update_log_cnt()
{
    log_cnt++;
    if (log_cnt == MAX_LOG_CNT)
    {
        printk("CUBIC: ram log buffer overflow\n");
        flush_log();
    }
}

EXPORT_SYMBOL(update_log_cnt);

