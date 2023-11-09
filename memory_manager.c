#include <linux/module.h>       // Needed by all modules
#include <linux/kernel.h>       // Needed for KERN_INFO
#include <linux/init.h>         // Needed for the macros
#include <linux/sched.h>        // Needed for task_struct and current
#include <linux/mm.h>           // Needed for mm_struct
#include <linux/hrtimer.h>      // Needed for high-resolution timers
#include <linux/ktime.h>        // Needed for ktime_* functions

static int pid;
module_param(pid, int, 0);

MODULE_LICENSE("GPL");

static int __init memory_manager_init(void)
{
    printk(KERN_INFO "\nInitializing memory-manager module\n");
    return 0;
}


static void __exit memory_manager_exit(void)
{
    printk(KERN_INFO "Exiting producer-consumer module\n");
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
