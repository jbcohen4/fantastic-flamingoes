#include <linux/module.h>       // Needed by all modules
#include <linux/kernel.h>       // Needed for KERN_INFO
#include <linux/init.h>         // Needed for the macros
#include <linux/sched.h>        // Needed for task_struct and current
#include <linux/mm.h>           // Needed for mm_struct
#include <linux/hrtimer.h>      // Needed for high-resolution timers
#include <linux/ktime.h>        // Needed for ktime_* functions

// Define module metadata
MODULE_LICENSE("GPL");
/*MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Memory Manager Module for CSE 330 Project 3");
MODULE_VERSION("0.1");*/

static int pid = -1; // Default to an invalid PID
module_param(pid, int, 0644); // Register pid as module parameter
//MODULE_PARM_DESC(pid, "Process ID");

// Function to check and clear the accessed bit of a PTE
static int my_ptep_test_and_clear_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep) {
    int ret = 0;
    if (pte_young(*ptep))
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
    return ret;
}

// High-resolution timer callback function
static enum hrtimer_restart timer_callback(struct hrtimer *timer) {
    struct memory_measure measure = {0};
    struct pid *pid_struct;
    struct task_struct *task;

    // Get the task_struct associated with the given PID
    rcu_read_lock();
    pid_struct = find_get_pid(pid);
    task = pid_task(pid_struct, PIDTYPE_PID);
    if (task) {
        get_task_struct(task); // Increment task's usage counter
        rcu_read_unlock();

        // Reset the measurement values
        measure.rss = 0;
        measure.swap = 0;
        measure.wss = 0;

        // Measure the memory usage
        walk_page_table(task, &measure);

        put_task_struct(task); // Decrement the task's usage counter

        // Output the memory usage measurements
        printk(KERN_INFO "PID %d: RSS=%lu KB, SWAP=%lu KB, WSS=%lu KB\n",
               pid, measure.rss / 1024, measure.swap / 1024, measure.wss / 1024);
    } else {
        rcu_read_unlock();
    }

    // Re-arm the timer for the next interval
    ktime_t interval = ktime_set(10, 0); // 10 seconds
    hrtimer_forward(timer, ktime_get(), interval);

    return HRTIMER_RESTART;
}

// Structure to hold measurement data
struct memory_measure {
    unsigned long rss; // Resident set size
    unsigned long swap; // Swap size
    unsigned long wss; // Working set size
};

// Function to walk the page table and update memory_measure structure
static void walk_page_table(struct task_struct *task, struct memory_measure *measure) {
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

            // If the pmd is not present, we assume it's swapped out
            if (!(pmd_present(*pmd))) {
                measure->swap += PAGE_SIZE;
                continue;
            }

            // Finally, get the pte
            pte = pte_offset_map(pmd, address);
            if (!pte)
                continue;
            if (pte_present(*pte)) {
                measure->rss += PAGE_SIZE; // Page is in resident set
                if (pte_young(*pte) || pte_dirty(*pte)) {
                    measure->wss += PAGE_SIZE; // Page is in working set
                }
            } else {
                measure->swap += PAGE_SIZE; // Page is in swap
            }
            pte_unmap(pte);
        }
    }
}

static struct hrtimer hr_timer; // Declare the high-resolution timer

// Module initialization function
static int __init memory_manager_init(void) {
    printk(KERN_INFO "Memory Manager Module loaded with PID: %d\n", pid);

    // Initialize high-resolution timer to fire every 10 seconds
    ktime_t ktime = ktime_set(10, 0); // 10 seconds
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

    // Other initialization work...
    
    return 0; // Return 0 for successful loading
}

// Module cleanup function
static void __exit memory_manager_exit(void) {
    int ret;

    ret = hrtimer_cancel(&hr_timer); // Cancel the high-resolution timer
    if (ret)
        printk(KERN_INFO "The timer was still in use...\n");
    printk(KERN_INFO "Memory Manager Module unloaded.\n");
    
    // Other cleanup work...
}

// Register initialization and cleanup functions
module_init(memory_manager_init);
module_exit(memory_manager_exit);

// Define additional functions here as needed for page table traversal and measurements...

