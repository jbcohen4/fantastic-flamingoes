#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/timekeeping.h>

// Module Metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A producer-consumer kernel module");
MODULE_VERSION("0.1");

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
static int producer(void *data) {
    struct task_struct *task;
    for_each_process(task) {
        if (down_interruptible(&empty)) break;
        down_interruptible(&mutex);
        // Critical Section: Add tasks with UID == uuid
        if (task->cred->uid.val == uuid) {
            buffer[in] = task;
            printk(KERN_INFO "[Producer] Produced Item at buffer index: %d for PID: %d", in, task->pid);
            in = (in + 1) % buffSize;
        }
        up(&mutex);
        up(&full);
    }
    return 0;
}

// Consumer function
static int consumer(void *data) {
    // implement the consumer
    // read task_struct from buffer
    // calculate elapsed time
    // print log messages
    // ...
    return 0;
}

static int __init producer_consumer_init(void) {
    printk(KERN_INFO "Initializing producer-consumer module\n");

    // Initialize buffer index variables
    in = 0;
    out = 0;

    // Initialize Semaphores
    sema_init(&mutex, 1);  // Mutual exclusion semaphore initialized to 1
    sema_init(&empty, buffSize);  // 'empty' slots initialized to buffSize
    sema_init(&full, 0);  // 'full' slots initialized to 0

    // Create Producer Thread if required
    if (prod == 1) {
        struct task_struct *prod_thread;
        prod_thread = kthread_run(producer, NULL, "producer-thread");
        if (IS_ERR(prod_thread)) {
            printk(KERN_INFO "Error creating producer thread\n");
            return PTR_ERR(prod_thread);
        }
    }

    // Create Consumer Threads
    for (int i = 0; i < cons; ++i) {
        struct task_struct *cons_thread;
        cons_thread = kthread_run(consumer, NULL, "consumer-thread-%d", i);
        if (IS_ERR(cons_thread)) {
            printk(KERN_INFO "Error creating consumer thread %d\n", i);
            return PTR_ERR(cons_thread);
        }
    }

    return 0;
}


static void __exit producer_consumer_exit(void) {
    // Signal all the semaphores
    // Stop all the threads
    // ...
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
