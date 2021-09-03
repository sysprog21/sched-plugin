/* Process Scheduler Module dealing with execution of custom scheduler */

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/workqueue.h>

MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Process scheduler module");
MODULE_LICENSE("GPL");

#define ALL_REG_PIDS (-100)

/* Enumeration for Process States */
enum process_state {
    S_CREATED = 0,   /* Process in Created State */
    S_RUNNING = 1,   /* Process in Running State */
    S_WAITING = 2,   /* Process in Waiting State */
    S_BLOCKED = 3,   /* Process in Blocked State */
    S_TERMINATED = 4 /* Process in Terminate State */
};

extern int add_process_to_queue(int pid);
extern int remove_process_from_queue(int pid);
extern int print_process_queue(void);
extern int change_process_state_in_queue(int pid, int changeState);
extern int get_first_process_in_queue(void);
extern int remove_terminated_processes_from_queue(void);

static void context_switch(struct work_struct *w);
static int static_round_robin_scheduling(void);

static int flag = 0;

/* Time Quantum storage variable for preemptive schedulers */
static int time_quantum = 3;

static int current_pid = -1;

struct workqueue_struct *scheduler_wq;

/* Create a delayed_work object with the provided function handler */
static DECLARE_DELAYED_WORK(scheduler_hdlr, context_switch);

/* switch the currently executing process with another process.
 * It internally calls the provided scheduling policy.
 */
static void context_switch(struct work_struct *w)
{
    /* Boolean status of the queue */
    bool q_status = false;

    printk(KERN_ALERT "Scheduler instance: Context Switch\n");

    /* Invoking the static round robin scheduling policy */
    static_round_robin_scheduling();

    /* Condition check for producer unloading flag set or not */
    if (flag == 0) {
        /* Setting the delayed work execution for the provided rate */
        q_status = queue_delayed_work(scheduler_wq, &scheduler_hdlr,
                                      time_quantum * HZ);
    } else
        printk(KERN_ALERT "Scheduler instance: scheduler is unloading\n");
}

static int static_round_robin_scheduling(void)
{
    /* Storage class variable to detecting the process state change */
    int ret_process_state = -1;

    printk(KERN_INFO "Static Round Robin Scheduling scheme.\n");

    /* Removing all terminated process from the queue */
    remove_terminated_processes_from_queue();

    /* Check if the current process id is INVALID or not */
    if (current_pid != -1) {
        /* Add the current process to the process queue */
        add_process_to_queue(current_pid);
    }

    /* Obtaining the first process in the wait queue */
    current_pid = get_first_process_in_queue();

    /* Check if the obtained process id is invalid or not. If Invalid
     * indicates, the queue does not contain any active process.
     */
    if (current_pid != -1) {
        /* Change the process state of the obtained process from queue to
         * running.
         */
        ret_process_state =
            change_process_state_in_queue(current_pid, S_RUNNING);

        /* Remove the process from the waiting queue */
        remove_process_from_queue(current_pid);
    }

    printk(KERN_INFO "Currently running process: %d\n", current_pid);

    /* Check if there no processes active in the scheduler or not */
    if (current_pid != -1) {
        printk(KERN_INFO "Current Process Queue...\n");
        print_process_queue();
        printk(KERN_INFO "Currently running process: %d\n", current_pid);
    }

    /* success */
    return 0;
}

static int __init process_scheduler_module_init(void)
{
    bool q_status = false;

    printk(KERN_INFO "Process Scheduler module is being loaded.\n");

    scheduler_wq = alloc_workqueue("scheduler-wq", WQ_UNBOUND, 1);

    if (scheduler_wq == NULL) {
        printk(KERN_ERR
               "Scheduler instance ERROR:Workqueue cannot be allocated\n");
        /** Memory Allocation Problem */
        return -ENOMEM;
    }

    /* Performing an internal call for context_switch */
    /** Setting the delayed work execution for the provided rate */
    q_status =
        queue_delayed_work(scheduler_wq, &scheduler_hdlr, time_quantum * HZ);
    return 0;
}

static void __exit process_scheduler_module_cleanup(void)
{
    /* Signalling the scheduler module unloading */
    flag = 1;

    /* Cancelling pending jobs in the Work Queue */
    cancel_delayed_work(&scheduler_hdlr);

    /* Removing all the pending jobs from the Work Queue */
    flush_workqueue(scheduler_wq);

    /* Deallocating the Work Queue */
    destroy_workqueue(scheduler_wq);
    printk(KERN_INFO "Process Scheduler module is being unloaded.\n");
}

module_init(process_scheduler_module_init);
module_exit(process_scheduler_module_cleanup);
module_param(time_quantum, int, 0);
