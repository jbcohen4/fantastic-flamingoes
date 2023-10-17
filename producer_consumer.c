#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/timekeeping.h>

// for some reason I think we need the license?
// it got mad at us when we didn't have it
MODULE_LICENSE("GPL");
static int buffSize;
static int prod;
static int cons;
static int uuid;

module_param(buffSize, int, 0);
module_param(prod, int, 0);
module_param(cons, int, 0);
module_param(uuid, int, 0);


static struct task_struct *prod_thread = NULL;
static struct task_struct **cons_threads = NULL;

struct semaphore mutex;
struct semaphore empty;
struct semaphore full;

static struct task_struct **buffer = NULL;
int in, out;

static int producer(void *data)
{
   struct task_struct *task;
    
   for_each_process(task)
    {
	if (down_interruptible(&empty))
	    break;

	if (down_interruptible(&mutex))
	{
	    up(&empty);
	    break;
	}

	if (task->cred->uid.val == uuid)
	{
	    buffer[in] = task;
	    printk(KERN_INFO "[Producer] Produced Item at buffer index: %d for PID: %d", in, task->pid);
	    in = (in + 1) % buffSize;

	    // one more slot has been filled
	    up(&full);
	}
	else
	{
	    // If we didn't add anything to the buffer, then we need to undo the down(&empty) that we did earlier
	    up(&empty);
	}
	up(&mutex);
    }
    
    while (!kthread_should_stop()){} // we don't want this thread to stop until we signal it to. 
    return 0;
}

static u64 total_elapsed_time = 0;

static int consumer(void *data)
{
    int consumer_id = (int)(uintptr_t)data;
    struct task_struct *task;
    u64 elapsed_time;

    while (!kthread_should_stop())
    {
        if (down_interruptible(&full))
            break;

        if (down_interruptible(&mutex))
        {
            up(&full);
            break;
        }

        task = buffer[out];
        out = (out + 1) % buffSize;

        up(&mutex);

        up(&empty);
        elapsed_time = ktime_get_ns() - task->start_time;
        total_elapsed_time += elapsed_time;
        
        elapsed_time /= 1000000000; // convert ns to seconds
        u64 min = (elapsed_time % 3600) / 60;
        u64 hours = elapsed_time / 3600;
        u64 seconds = elapsed_time % 60;

        printk(KERN_INFO "[Consumer-%d] Consumed Item#-%d on buffer index:%d PID:%d Elapsed Time-%02llu:%02llu:%02llu\n",
               (int)data, out, out, task->pid, hours, min, seconds);
    }

    return 0;
}

static int __init producer_consumer_init(void)
{
    printk(KERN_INFO "\nInitializing producer-consumer module\n");

    cons_threads = kmalloc(sizeof(struct task_struct *) * cons, GFP_KERNEL);
    if (!cons_threads)
    {
        printk(KERN_INFO "Error allocating memory for consumer threads\n");
        return -ENOMEM;
    }

    buffer = kmalloc(sizeof(struct task_struct *) * buffSize, GFP_KERNEL);
    if (!buffer)
    {
        printk(KERN_INFO "Error allocating memory for buffer\n");
        kfree(cons_threads); // free the memory if there was an allocation error
        return -ENOMEM;
    }

    in = 0;
    out = 0;

    sema_init(&mutex, 1);
    sema_init(&empty, buffSize);
    sema_init(&full, 0);

    if (prod == 1)
    {
        prod_thread = kthread_run(producer, NULL, "producer-thread");
        if (IS_ERR(prod_thread))
        {
            printk(KERN_INFO "Error creating producer thread\n");
            return PTR_ERR(prod_thread);
        }
    }

    for (int i = 0; i < cons; ++i)
    {
        cons_threads[i] = kthread_run(consumer, (void *)(uintptr_t)i, "consumer-thread-%d", i);
        if (IS_ERR(cons_threads[i]))
        {
            printk(KERN_INFO "Error creating consumer thread %d\n", i);
            return PTR_ERR(cons_threads[i]);
        }
    }

    return 0;
}

static void __exit producer_consumer_exit(void)
{
    printk(KERN_INFO "Exiting producer-consumer module\n");

    // kill the producer thread
    // this is why we need to keep the producer thread alive until we send the kill signal (here)
    if (prod_thread && !IS_ERR(prod_thread))
    {
        kthread_stop(prod_thread);
    }

    // kill all the consumer threads
    for (int i = 0; i < cons; i++)
    {
        if (cons_threads[i] && !IS_ERR(cons_threads[i]))
            kthread_stop(cons_threads[i]);
    }

    // free the memory we kmalloc'd
    kfree(cons_threads);
    kfree(buffer);

    total_elapsed_time /= 1000000000;
    u64 min = (total_elapsed_time % 3600) / 60;
    u64 hours = total_elapsed_time / 3600;
    u64 seconds = total_elapsed_time % 60;
    printk(KERN_INFO "Total elapsed time: %02llu:%02llu:%02llu\n", hours, min, seconds);
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
