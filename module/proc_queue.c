/* Process Queue Module dealing with the handling aspects of storage and
 * retrieval of process information about a given process.
 */

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/time.h>

MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Process queue module");
MODULE_LICENSE("GPL");

#define ALL_REG_PIDS (-100)
#define INVALID_PID (-1)

/* Enumeration for Process States */
enum process_state {
    S_CREATED = 0,   /* Process in Created State */
    S_RUNNING = 1,   /* Process in Running State */
    S_WAITING = 2,   /* Process in Waiting State */
    S_BLOCKING = 3,  /* Process in Blocked State */
    S_TERMINATED = 4 /* Process in Terminate State */
};

/* Enumeration for Task Errors */
enum task_status_code {
    TS_EXIST = 0,      /* Task is still active */
    TS_TERMINATED = -1 /* Task has terminated */
};

/* Structure for a process */
struct proc {
    int pid;                  /* Process ID */
    enum process_state state; /* Process State */
    struct list_head list;    /* List pointer for generating a list of proc */
    /* FIXME: More things to come in future such as nice value and prio. */
} top;

/* Semaphore for process queue */
static struct semaphore mutex;

enum task_status_code task_status_change(int pid, enum process_state eState);
enum task_status_code is_task_exists(int pid);

int init_process_queue(void);
int release_process_queue(void);
int add_process_to_queue(int pid);
int remove_process_from_queue(int pid);
int print_process_queue(void);
int change_process_state_in_queue(int pid, int changeState);
int get_first_process_in_queue(void);
int remove_terminated_processes_from_queue(void);

/* initialize a process queue */
int init_process_queue(void)
{
    printk(KERN_INFO "Initializing the Process Queue...\n");

    /* Generate the head of the queue and initializing an empty proc queue */
    INIT_LIST_HEAD(&top.list);
    return 0;
}

/* release a process queue */
int release_process_queue(void)
{
    struct proc *tmp, *node;
    printk(KERN_INFO "Releasing Process Queue...\n");

    /* Iterate over the list of nodes pertaining to the process information
     * and remove one by one.
     */
    list_for_each_entry_safe (node, tmp, &(top.list), list) {
        /* Deleting link pointer established by the node to the list */
        list_del(&node->list);
        /* Removing the whole node */
        kfree(node);
    }
    /* success */
    return 0;
}

/* add a process into a queue */
int add_process_to_queue(int pid)
{
    /* Allocating space for the newly registered process */
    struct proc *new_process = kmalloc(sizeof(struct proc), GFP_KERNEL);

    /* Check if the kmalloc call was successful or not */
    if (!new_process) {
        printk(KERN_ALERT
               "Process Queue ERROR: kmalloc function failed from "
               "add_process_to_queue function.");
        /* Add process to queue error */
        return -ENOMEM;
    }

    /* Setting the process id to the process info node new_process */
    new_process->pid = pid;

    /* Setting process state to the process info node new_process as waiting */
    new_process->state = S_WAITING;

    /* Make the task level alteration therefore the process pauses its execution
     * since in wait state.
     */
    task_status_change(new_process->pid, new_process->state);
    /* TODO: add error handling */

    /* Condition to verify the down operation on the binary semaphore.
     * Entry into a Mutually exclusive block is granted by having a successful
     * lock with the mentioned semaphore.
     * binary semaphore provides a safe access to following critical section.
     */
    if (down_interruptible(&mutex)) {
        printk(KERN_ALERT
               "Process Queue ERROR:Mutual Exclusive position access failed "
               "from add function");
        /* Issue a restart of syscall which was supposed to be executed */
        return -ERESTARTSYS;
    }

    /* Initialize the new process list as the new head. */
    INIT_LIST_HEAD(&new_process->list);

    /* Set the new process as a tail to the previous top of the list */
    list_add_tail(&(new_process->list), &(top.list));

    /* Such an operation indicates the critical section is released for other
     * processes/threads.
     */
    up(&mutex);

    printk(KERN_INFO "Adding the given Process %d to the  Process Queue...\n",
           pid);
    /* success */
    return 0;
}

/* remove a specified process from the queue */
int remove_process_from_queue(int pid)
{
    struct proc *tmp, *node;
    if (down_interruptible(&mutex)) {
        printk(KERN_ALERT
               "Process Queue ERROR:Mutual Exclusive position access failed "
               "from remove function");
        /* Issue a restart of syscall which was supposed to be executed */
        return -ERESTARTSYS;
    }

    /* Iterate over process queue and remove the process with provided PID */
    list_for_each_entry_safe (node, tmp, &(top.list), list) {
        /** Check if the node pid is the same as the required pid */
        if (node->pid == pid) {
            printk(KERN_INFO
                   "Removing the given Process %d from the  Process Queue...\n",
                   pid);
            /* Deleting link pointer established by the node to the list */
            list_del(&node->list);

            /* Removing the whole node */
            kfree(node);
        }
    }

    up(&mutex);
    /* success */
    return 0;
}

/* remove all terminated processes from the queue */
int remove_terminated_processes_from_queue(void)
{
    struct proc *tmp, *node;
    if (down_interruptible(&mutex)) {
        printk(KERN_ALERT
               "Process Queue ERROR:Mutual Exclusive position access failed "
               "from remove function");
        /* Issue a restart of syscall which was supposed to be executed */
        return -ERESTARTSYS;
    }
    /* Iterate over proc queue and remove all terminated processes from queue */
    list_for_each_entry_safe (node, tmp, &(top.list), list) {
        /* Check if the process is terminated or not */
        if (node->state == S_TERMINATED) {
            printk(KERN_INFO
                   "Removing the terminated Process %d from the Process "
                   "Queue...\n",
                   node->pid);
            /* Delete link pointer established by the node to the list */
            list_del(&node->list);
            /* Remove the whole node */
            kfree(node);
        }
    }

    up(&mutex);
    /* success */
    return 0;
}

/* change the process state for a given process in the queue */
int change_process_state_in_queue(int pid, int changeState)
{
    struct proc *tmp, *node;

    enum process_state ret_process_change_status;

    if (down_interruptible(&mutex)) {
        printk(KERN_ALERT
               "Process Queue ERROR:Mutual Exclusive position access failed "
               "from change process state function");
        /* Issue a restart of syscall which was supposed to be executed */
        return -ERESTARTSYS;
    }
    /* Check if all registered PIDs are modified for state */
    if (pid == ALL_REG_PIDS) {
        list_for_each_entry_safe (node, tmp, &(top.list), list) {
            printk(KERN_INFO
                   "Updating the process state the Process %d in  Process "
                   "Queue...\n",
                   node->pid);
            /* Update the state to the provided state */
            node->state = changeState;
            /* Check if the task associated with iterated node still exists */
            if (task_status_change(node->pid, node->state) == TS_TERMINATED) {
                node->state = S_TERMINATED;
            }
        }
    } else {
        list_for_each_entry_safe (node, tmp, &(top.list), list) {
            /**Check if the iterated node is the required process or not.*/
            if (node->pid == pid) {
                printk(KERN_INFO
                       "Updating the process state the Process %d in  Process "
                       "Queue...\n",
                       pid);
                node->state = changeState;
                if (task_status_change(node->pid, node->state) ==
                    TS_TERMINATED) {
                    node->state = S_TERMINATED;
                    /* Return value updated to notify that the requested process
                     * is already terminated.
                     */
                    ret_process_change_status = S_TERMINATED;
                }
            } else {
                /* Check if task associated with the iterated node exists */
                if (is_task_exists(node->pid) == TS_TERMINATED) {
                    node->state = S_TERMINATED;
                }
            }
        }
    }

    up(&mutex);

    /* Return the process status change associated with the internal call to
     * task status change method.
     */
    return ret_process_change_status;
}


int print_process_queue(void)
{
    struct proc *tmp;
    printk(KERN_INFO "Process Queue: \n");
    if (down_interruptible(&mutex)) {
        printk(KERN_ALERT
               "Process Queue ERROR:Mutual Exclusive position access failed "
               "from print function");
        return -ERESTARTSYS;
    }

    list_for_each_entry (tmp, &(top.list), list) {
        printk(KERN_INFO "Process ID: %d\n", tmp->pid);
    }

    up(&mutex);
    return 0;
}

int get_first_process_in_queue(void)
{
    struct proc *tmp;
    /* Initially set the process id value as an INVALID value */
    int pid = INVALID_PID;

    if (down_interruptible(&mutex)) {
        printk(KERN_ALERT
               "Process Queue ERROR:Mutual Exclusive position access failed "
               "from print function");
        /* Issue a restart of syscall which was supposed to be executed */
        return -ERESTARTSYS;
    }

    /* Iterate over the process queue and find the first active process */
    list_for_each_entry (tmp, &(top.list), list) {
        /* Check if the task associated with the process is terminated */
        if ((pid == INVALID_PID) && (is_task_exists(tmp->pid) == TS_EXIST)) {
            /* Set the process id to read process */
            pid = tmp->pid;
        }
    }

    up(&mutex);

    /* Returns the first process ID */
    return pid;
}

enum task_status_code is_task_exists(int pid)
{
    struct task_struct *current_pr;
    current_pr = pid_task(find_vpid(pid), PIDTYPE_PID);
    /* Check if the task exists or not by checking for NULL Value */
    if (current_pr == NULL) {
        /* Return the task status code as terminated */
        return TS_TERMINATED;
    }
    /* Return the task status code as exists */
    return TS_EXIST;
}

enum task_status_code task_status_change(int pid, enum process_state eState)
{
    struct task_struct *current_pr;
    /* Obtain the task struct associated with provided PID */
    current_pr = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (current_pr == NULL) {
        return TS_TERMINATED;
    }

    /* Check if the state change was Running */
    if (eState == S_RUNNING) {
        /* Trigger a signal to continue the given task associated with the
         * process.
         */
        kill_pid(task_pid(current_pr), SIGCONT, 1);
        printk(KERN_INFO "Task status change to Running\n");
    } else if (eState == S_WAITING) { /* if state change was Waiting */
        /* Trigger a signal to pause the given task associated with the
         * process.
         */
        kill_pid(task_pid(current_pr), SIGSTOP, 1);
        printk(KERN_INFO "Task status change to Waiting\n");
    } else if (eState == S_BLOCKING) { /* if state change was Blocked */
        printk(KERN_INFO "Task status change to Blocked\n");
    } else if (eState == S_TERMINATED) { /* if state change was Terminated */
        printk(KERN_INFO "Task status change to Terminated\n");
    }

    /* Return the task status code as exists */
    return TS_EXIST;
}

static int __init process_queue_module_init(void)
{
    printk(KERN_INFO "Process Queue module is being loaded.\n");
    sema_init(&mutex, 1);

    init_process_queue();
    return 0;
}

static void __exit process_queue_module_cleanup(void)
{
    printk(KERN_INFO "Process Queue module is being unloaded.\n");
    release_process_queue();
}

module_init(process_queue_module_init);
module_exit(process_queue_module_cleanup);

EXPORT_SYMBOL_GPL(init_process_queue);
EXPORT_SYMBOL_GPL(release_process_queue);
EXPORT_SYMBOL_GPL(add_process_to_queue);
EXPORT_SYMBOL_GPL(remove_process_from_queue);
EXPORT_SYMBOL_GPL(print_process_queue);
EXPORT_SYMBOL_GPL(get_first_process_in_queue);
EXPORT_SYMBOL_GPL(change_process_state_in_queue);
EXPORT_SYMBOL_GPL(remove_terminated_processes_from_queue);
