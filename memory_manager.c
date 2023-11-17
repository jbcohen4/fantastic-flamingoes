#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

static int pid;
module_param(pid, int, 0);

MODULE_LICENSE("GPL");

static struct hrtimer hr_timer;
static unsigned long timer_interval_ns = 10000000000UL; // 10 seconds in nanoseconds

enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    ktime_t now, interval;

    now = ktime_get();
    interval = ktime_set(0, timer_interval_ns);
    hrtimer_forward(timer, now, interval);

    // Reduce the frequency of logging
    pr_info_ratelimited("Timer callback\n");

    return HRTIMER_RESTART;
}

static int __init memory_manager_init(void)
{
    ktime_t ktime;

    pr_info("Memory Manager Module loaded with PID: %d\n", pid);

    ktime = ktime_set(0, timer_interval_ns);
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
    hr_timer.function = timer_callback;

    return 0;
}

static void __exit memory_manager_exit(void)
{
    int ret;

    ret = hrtimer_cancel(&hr_timer);
    if (ret)
        pr_info("The timer was still in use...\n");

    pr_info("Memory Manager Module unloaded.\n");
}

module_init(memory_manager_init);
module_exit(memory_manager_exit);
