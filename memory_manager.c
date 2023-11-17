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


int ptep_test_and_clear_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep)
{
    int ret = 0;
    if (pte_young(*ptep))
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
    return ret; 
}

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

typedef struct {
    unsigned long rss;
    unsigned long swap;
    unsigned long wss;
} MemoryMeasure;

static MemoryMeasure get_memory_usage(struct task_struct *task) {
    MemoryMeasure result;
    result.rss = 0;
    result.swap = 0;
    result.wss = 0;

    struct mm_struct *mm = task->mm;
    struct vm_area_struct *vma = NULL;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    // Walk through all VMAs of the process
    for (vma = mm->mmap ; vma; vma = vma->vm_next) {
        unsigned long address = vma->vm_start;

        // Walk through each page in the VMA
        for (; address < vma->vm_end; address += PAGE_SIZE) {
            pgd = pgd_offset(mm, address);
            if (pgd_none(*pgd) || pgd_bad(*pgd))
                continue;

            p4d = p4d_offset(pgd, address);
            if (p4d_none(*p4d) || p4d_bad(*p4d))
                continue;

            pud = pud_offset(p4d, address);
            if (pud_none(*pud) || pud_bad(*pud))
                continue;

            pmd = pmd_offset(pud, address);
            if (pmd_none(*pmd) || pmd_bad(*pmd))
                continue;

            pte = pte_offset_map(pmd, address);
            if (!pte)
                continue;
            if (pte_present(*pte)) {
                result.rss += PAGE_SIZE; // Page is in resident set
                if (pte_young(*pte) || pte_dirty(*pte)) {
                    result.wss += PAGE_SIZE; // Page is in working set
                }
            } else {
                result.swap += PAGE_SIZE; // Page is in swap
            }
            pte_unmap(pte);
        }
    }
    return result;
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


