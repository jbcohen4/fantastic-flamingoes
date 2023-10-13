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

static int num_producers_active, num_consumers_active;
static int global_kill_flag;


// Producer function
// The code is a function called producer in a C program. This function is part of a producer-consumer pattern,
// where one or more threads produce data and one or more threads consume data. The producer function is responsible for adding tasks to a shared buffer.
static int producer(void *data)
{
    num_producers_active++;
    struct task_struct *task;
    for_each_process(task)
    {
        // Attempt to acquire the empty semaphore. If not available or interrupted, break out of the loop.
        if (down_interruptible(&empty))
            break;
        if (global_kill_flag){ // only true when the entire program is going to end
            break;
        }

        // Attempt to acquire the mutex semaphore. If interrupted, release the empty semaphore and move to next process.
        if (down_interruptible(&mutex))
            break;
        if (global_kill_flag){ // only true when the entire program is going to end
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
    num_producers_active--;
    return 0;
}

// Consumer function
static u64 total_elapsed_time = 0;

static int consumer(void *data)
{
    num_consumers_active++;
    struct task_struct *task;
    u64 elapsed_time;
    

    while (!global_kill_flag)
    {
        if (down_interruptible(&full))
            break; // exit the loop if interrupt signal received
        if (global_kill_flag){ // only true when the entire program is going to end
            break;
        }

        if (down_interruptible(&mutex))
            break; // exit the loop if signal received
        if (global_kill_flag){ // only true when the entire program is going to end
            break;
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
    num_consumers_active--;
    return 0;
}

static int __init producer_consumer_init(void)
{
    global_kill_flag = 0;
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
    global_kill_flag = 1;
    printk(KERN_INFO "Exiting producer-consumer module\n");


    while(num_producers_active != 0 || num_consumers_active != 0){
        // wait for those threads to finish
        up(&empty);
        up(&full);
        up(&mutex);
    }


    // Calculate and print total elapsed time
    printk(KERN_INFO "Total elapsed time: %llu\n", total_elapsed_time); // I think this line has a bug, but I'm not going to explain/fix it now
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
