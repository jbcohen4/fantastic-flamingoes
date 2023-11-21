#include <linux/module.h>       // Needed by all modules
#include <linux/kernel.h>       // Needed for KERN_INFO
#include <linux/init.h>         // Needed for the macros
#include <linux/sched.h>        // Needed for task_struct and current
#include <linux/mm.h>           // Needed for mm_struct
#include <linux/mm_types.h>     // Needed for mm_struct
#include <linux/hrtimer.h>      // Needed for high-resolution timers
#include <linux/ktime.h>        // Needed for ktime_* functions
#include <linux/hrtimer.h>      // Needed for the timer callback to work




typedef struct {
    unsigned long rss;
    unsigned long swap;
    unsigned long wss;
} MemoryMeasure;


int ptep_test_and_clear_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep){
    int ret = 0;
    if (pte_young(*ptep))
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
    return ret; 
}

int simple_ptep_test_and_clear_young(pte_t *ptep){
    int ret = 0;
    if (pte_young(*ptep))
        ret = test_and_clear_bit(_PAGE_BIT_ACCESSED, (unsigned long *) &ptep->pte);
    return ret; 
}

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
            if (!pte_none(*pte)){
                if (pte_present(*pte) == 1) {
                    result.rss += PAGE_SIZE; // Page is in resident set
                    if (simple_ptep_test_and_clear_young(pte)) {
                        result.wss += PAGE_SIZE; // Page is in working set
                    }
                } else {
                    result.swap += PAGE_SIZE; // Page is in swap
                }
            }
            pte_unmap(pte);
        }
    }
    return result;
}




static int pid;
module_param(pid, int, 0);

MODULE_LICENSE("GPL");


static struct hrtimer hr_timer;
ktime_t ktime;

static enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer){
    // Get the task_struct associated with the given PID
    rcu_read_lock();
    struct pid *pid_struct = find_get_pid(pid);
    struct task_struct *task = pid_task(pid_struct, PIDTYPE_PID);

    MemoryMeasure result = get_memory_usage(task);


    printk(KERN_INFO "PID %d: RSS=%ld KB, SWAP=%ld KB, WSS=%ld KB", pid, result.rss/1024, result.swap/1024, result.wss/1024);

    hrtimer_forward_now(timer, ktime);
    return HRTIMER_RESTART;
}




static int __init memory_manager_init(void){
    ktime = ktime_set(10, 0);
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hr_timer.function = &my_hrtimer_callback;
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

    printk(KERN_INFO "Memory Manager Module loaded with PID: %d\n", pid);

    return 0;
}


static void __exit memory_manager_exit(void){
    int ret = hrtimer_cancel(&hr_timer);
    printk(KERN_INFO "Memory Manager Module unloaded.\n");
}




module_init(memory_manager_init);
module_exit(memory_manager_exit);


