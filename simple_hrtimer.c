#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple HR Timer Kernel Module");

static struct hrtimer hr_timer;
ktime_t ktime;

static enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer)
{
    printk(KERN_INFO "HR Timer callback executed\n");
    return HRTIMER_NORESTART;
}

static int __init my_module_init(void)
{
    ktime = ktime_set(0, 10000000); // 10 ms
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hr_timer.function = &my_hrtimer_callback;
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

    printk(KERN_INFO "HR Timer module installed\n");
    return 0;
}

static void __exit my_module_exit(void)
{
    int ret = hrtimer_cancel(&hr_timer);
    if (ret) printk(KERN_INFO "HR Timer still in use\n");
    printk(KERN_INFO "HR Timer module removed\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
