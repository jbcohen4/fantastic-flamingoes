#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/timekeeping.h>

// Module Parameters
static int buffSize;
static int prod;
static int cons;
static int uuid;
module_param(buffSize, int, 0);
module_param(prod, int, 0);
module_param(cons, int, 0);
module_param(uuid, int, 0);

// Semaphores
struct semaphore mutex;
struct semaphore empty;
struct semaphore full;

// Shared Buffer
struct task_struct *buffer[100]; // Adjust this as needed
int in, out;

// Producer function
// The code above is a function called producer in a C program. This function is part of a producer-consumer pattern,
// where one or more threads produce data and one or more threads consume data. The producer function is responsible for adding tasks to a shared buffer.

static int producer(void *data)
{
    struct task_struct *task;
    // The function starts by iterating over all the processes in the system using the for_each_process macro.
    for_each_process(task)
    {
        // For each process, the function checks if the empty semaphore is available using the down_interruptible function.
        // If the semaphore is not available, the function breaks out of the loop.
        if (down_interruptible(&empty))
            break;
        // If the empty semaphore is available, the function acquires the mutex semaphore using the down_interruptible function.
        // This semaphore is used to protect a critical section of code, which is a section of code that must be executed atomically,
        // without interruption from other threads or processes.

        down_interruptible(&mutex);
        // Critical Section: Add tasks with UID == uuid
        // Inside the critical section, the function checks if the UID of the current process matches a given UUID.
        // If the UID matches, the function adds the task to the shared buffer at the current index in.
        // The function then prints a message to the kernel log indicating that an item has been produced at the current buffer index for the given PID.
        if (task->cred->uid.val == uuid)
        {
            buffer[in] = task;
            printk(KERN_INFO "[Producer] Produced Item at buffer index: %d for PID: %d", in, task->pid);

            // After adding the task to the buffer, the function updates the index in to point to the next available buffer slot using the modulo operator.

            in = (in + 1) % buffSize;
        }
        // Finally, the function releases the mutex semaphore using the up function and signals that the buffer is no longer empty by releasing the full semaphore using the up function.

        up(&mutex);
        up(&full);
    }
    return 0;
}

// Consumer function
static u64 total_elapsed_time = 0;

static int __init producer_consumer_init(void)
{
    printk(KERN_INFO "Initializing producer-consumer module\n");

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
        struct task_struct *prod_thread;
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
        struct task_struct *cons_thread;
        cons_thread = kthread_run(consumer, NULL, "consumer-thread-%d", i);
        if (IS_ERR(cons_thread))
        {
            printk(KERN_INFO "Error creating consumer thread %d\n", i);
            return PTR_ERR(cons_thread);
        }
    }

    return 0;
}

static void __exit producer_consumer_exit(void)
{
    // Signal all the semaphores
    // Stop all the threads
    // ...
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
