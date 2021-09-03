/* Accessing a proc system file which set the process ID which can be later
 * used for our custom scheduler.
 */

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Process setting module");
MODULE_LICENSE("GPL");

#define PROC_CONFIG_FILE_NAME "process_sched_add"
#define BASE_10 (10)

/* Enumeration for Process States */
enum process_state {
    S_CREATED = 0,   /**Process in Created State*/
    S_RUNNING = 1,   /**Process in Running State*/
    S_WAITING = 2,   /**Process in Waiting State*/
    S_BLOCKING = 3,  /**Process in Blocked State*/
    S_TERMINATED = 4 /**Process in Terminate State*/
};

/* Enumeration for Function Execution */
enum execution {
    EC_FAILED = -1, /* Function executed failed */
    EC_SUCCESS = 0  /* Function executed successfully */
};

static struct proc_dir_entry *proc_sched_add_file_entry;

extern int add_process_to_queue(int pid);
extern int remove_process_from_queue(int pid);
extern int print_process_queue(void);
extern int get_first_process_in_queue(void);
extern int remove_terminated_processes_from_queue(void);
extern int change_process_state_in_queue(int pid, int changeState);

static ssize_t process_sched_add_module_read(struct file *file,
                                             char *buf,
                                             size_t count,
                                             loff_t *ppos)
{
    printk(KERN_INFO "Process Scheduler Add Module read.\n");
    printk(KERN_INFO "Next Executable PID in the list if RR Scheduling: %d\n",
           get_first_process_in_queue());
    /* Successful execution of read call back. EOF reached */
    return 0;
}

static ssize_t process_sched_add_module_write(struct file *file,
                                              const char *buf,
                                              size_t count,
                                              loff_t *ppos)
{
    int ret;
    long int new_proc_id;

    printk(KERN_INFO "Process Scheduler Add Module write.\n");
    printk(KERN_INFO "Registered Process ID: %s\n", buf);

    ret = kstrtol(buf, BASE_10, &new_proc_id);
    if (ret < 0) {
        /* Invalid argument in conversion error */
        return -EINVAL;
    }

    /* Add process to the process queue */
    ret = add_process_to_queue(new_proc_id);

    /* Check if the add process to queue method was successful */
    if (ret != EC_SUCCESS) {
        printk(KERN_ALERT
               "Process Set ERROR:add_process_to_queue function failed from "
               "sched set write method");
        /* Add process to queue error */
        return -ENOMEM;
    }

    /* Successful execution of write call back */
    return count;
}

static int process_sched_add_module_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Process Scheduler Add Module open.\n");

    /** Successful execution of open call back.*/
    return 0;
}

static int process_sched_add_module_release(struct inode *inode,
                                            struct file *file)
{
    printk(KERN_INFO "Process Scheduler Add Module released.\n");
    /* Successful execution of release callback */
    return 0;
}

/* File operations related to process_sched_add file */
#ifdef HAVE_PROC_OPS
static struct proc_ops process_sched_add_module_fops = {
    .proc_read = process_sched_add_module_read,
    .proc_write = process_sched_add_module_write,
    .proc_open = process_sched_add_module_open,
    .proc_release = process_sched_add_module_release,
};
#else
static struct file_operations process_sched_add_module_fops = {
    .owner = THIS_MODULE,
    .read = process_sched_add_module_read,
    .write = process_sched_add_module_write,
    .open = process_sched_add_module_open,
    .release = process_sched_add_module_release,
};
#endif

static int __init process_sched_add_module_init(void)
{
    printk(KERN_INFO "Process Add to Scheduler module is being loaded.\n");

    /* created with RD + WR permissions with name process_sched_add */
    proc_sched_add_file_entry = proc_create(PROC_CONFIG_FILE_NAME, 0777, NULL,
                                            &process_sched_add_module_fops);
    /* Condition to verify if process_sched_add creation was successful */
    if (proc_sched_add_file_entry == NULL) {
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
               PROC_CONFIG_FILE_NAME);
        /* File Creation problem */
        return -ENOMEM;
    }

    return 0;
}

static void __exit process_sched_add_module_cleanup(void)
{
    printk(KERN_INFO "Process Add to Scheduler module is being unloaded.\n");
    proc_remove(proc_sched_add_file_entry);
}

module_init(process_sched_add_module_init);
module_exit(process_sched_add_module_cleanup);
