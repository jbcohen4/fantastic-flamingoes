#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pid.h>
#include <linux/sched.h>

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
