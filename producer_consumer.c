#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/timekeeping.h>

// Module Parameters
MODULE_LICENSE("GPL");
// Module Liscense
static int buffSize;
static int prod;
static int cons;
static int uuid;
module_param(buffSize, int, 0);
module_param(prod, int, 0);
module_param(cons, int, 0);
module_param(uuid, int, 0);
static struct task_struct *prod_thread = NULL; // Producer thread
static struct task_struct **cons_threads = NULL;
// Semaphores
struct semaphore mutex;
struct semaphore empty;
struct semaphore full;

// Shared Buffer
static struct task_struct **buffer = NULL;
int in, out;

// Producer function
// The code is a function called producer in a C program. This function is part of a producer-consumer pattern,
// where one or more threads produce data and one or more threads consume data. The producer function is responsible for adding tasks to a shared buffer.
static int producer(void *data)
{
    struct task_struct *task;
    while (!kthread_should_stop())
    {

        for_each_process(task)
        {
            // Attempt to acquire the empty semaphore. If not available or interrupted, break out of the loop.
            if (down_interruptible(&empty))
                break;

            // Attempt to acquire the mutex semaphore. If interrupted, release the empty semaphore and move to next process.
            if (down_interruptible(&mutex))
            {
                up(&empty);
                break;
            }

            // Critical Section: Add tasks with UID == uuid
            if (task->cred->uid.val == uuid)
            {
                buffer[in] = task;
                printk(KERN_INFO "[Producer] Produced Item at buffer index: %d for PID: %d", in, task->pid);
                in = (in + 1) % buffSize;

                // Signal that the buffer has an item.
                up(&full);
            }
            else
            {
                // If task was not added to the buffer, release the empty semaphore.
                up(&empty);
            }

            // Release the mutex semaphore.
            up(&mutex);
        }
    }
    return 0;
}

// Consumer function
static u64 total_elapsed_time = 0;

static int consumer(void *data)
{
    struct task_struct *task;
    u64 elapsed_time;

    while (!kthread_should_stop())
    {
        if (down_interruptible(&full))
            break; // exit the loop if interrupt signal received

        if (down_interruptible(&mutex))
        {
            up(&full); // release the full semaphore if interrupted
            break;     // exit the loop if signal received
        }

        // Read from the buffer
        task = buffer[out];
        out = (out + 1) % buffSize; // Circular buffer

        up(&mutex);

        // Signal that a buffer slot has become empty
        up(&empty);

        // Calculate elapsed time
        elapsed_time = ktime_get_ns() - task->start_time;
        elapsed_time /= 1000000000; // Convert to seconds

        // Update total elapsed time
        total_elapsed_time += elapsed_time;

        // Log the consumed item and elapsed time
        printk(KERN_INFO "[Consumer-%d] Consumed Item#-%d on buffer index:%d PID:%d Elapsed Time-%llu\n",
               (int)data, out, out, task->pid, elapsed_time);
    }

    return 0;
}

static int __init producer_consumer_init(void)
{
    printk(KERN_INFO "Initializing producer-consumer module\n");

    // Allocate memory for cons_threads based on the cons value
    cons_threads = kmalloc(sizeof(struct task_struct *) * cons, GFP_KERNEL);
    if (!cons_threads)
    {
        printk(KERN_INFO "Error allocating memory for consumer threads\n");
        return -ENOMEM;
    }

    // Allocate memory for buffer based on the buffSize value
    buffer = kmalloc(sizeof(struct task_struct *) * buffSize, GFP_KERNEL);
    if (!buffer)
    {
        printk(KERN_INFO "Error allocating memory for buffer\n");
        kfree(cons_threads); // Free memory allocated for consumer threads before returning
        return -ENOMEM;
    }

    // Initialize buffer index variables
    in = 0;
    out = 0;

    // Initialize Semaphores
    sema_init(&mutex, 1);        // Mutual exclusion semaphore initialized to 1
    sema_init(&empty, buffSize); // 'empty' slots initialized to buffSize
    sema_init(&full, 0);         // 'full' slots initialized to 0

    // Create Producer Thread if required
    if (prod == 1)
    {
        prod_thread = kthread_run(producer, NULL, "producer-thread");
        if (IS_ERR(prod_thread))
        {
            printk(KERN_INFO "Error creating producer thread\n");
            return PTR_ERR(prod_thread);
        }
    }

    // Create Consumer Threads
    for (int i = 0; i < cons; ++i)
    {
        cons_threads[i] = kthread_run(consumer, NULL, "consumer-thread-%d", i);
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

    // Stop all the threads
    if (prod_thread && !IS_ERR(prod_thread))
    {
        kthread_stop(prod_thread);
    }

    for (int i = 0; i < cons; i++)
    {
        if (cons_threads[i] && !IS_ERR(cons_threads[i]))
            kthread_stop(cons_threads[i]);
    }

    // Free dynamically allocated memory
    kfree(cons_threads);
    kfree(buffer);

    // Calculate and print total elapsed time
    printk(KERN_INFO "Total elapsed time: %llu\n", total_elapsed_time);
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
