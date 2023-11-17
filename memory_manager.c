#include <linux/module.h>       // Needed by all modules
#include <linux/kernel.h>       // Needed for KERN_INFO
#include <linux/init.h>         // Needed for the macros
#include <linux/sched.h>        // Needed for task_struct and current
#include <linux/mm.h>           // Needed for mm_struct
#include <linux/mm_types.h>     // Needed for mm_struct
#include <linux/hrtimer.h>      // Needed for high-resolution timers
#include <linux/ktime.h>        // Needed for ktime_* functions
#include <linux/hrtimer.h>      // Needed for the timer callback to work

static int pid;
module_param(pid, int, 0);

MODULE_LICENSE("GPL");


static struct hrtimer hr_timer; // Declare the high-resolution timer

static int __init memory_manager_init(void)
{
    printk(KERN_INFO "Memory Manager Module loaded with PID: %d\n", pid);

    // Initialize high-resolution timer to fire every 10 seconds
    ktime_t ktime = ktime_set(10, 0); // 10 seconds
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);


    return 0; // Return 0 for successful loading
}


// Module cleanup function
static void __exit memory_manager_exit(void)
{
    // int ret;

    // ret = hrtimer_cancel(&hr_timer); // Cancel the high-resolution timer
    // if (ret)
    //     printk(KERN_INFO "The timer was still in use...\n");
    printk(KERN_INFO "Memory Manager Module unloaded.\n");

    // Other cleanup work...
}


unsigned long timer_interval_ns= 10e9;
enum hrtimer_restart no_restart_callback(struct hrtimer *timer)
{
	ktime_t currtime, interval;
	currtime = ktime_get();
	interval = ktime_set(0, timer_interval_ns);
	hrtimer_forward(timer,currtime, interval);
	
    printk(KERN_INFO "It's working!\n");

	return HRTIMER_NORESTART;
}

module_init(memory_manager_init);
module_exit(memory_manager_exit);
