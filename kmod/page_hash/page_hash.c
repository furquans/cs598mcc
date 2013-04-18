// Copyright 2013 Zachary Estrada and Furquan Shaikh

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/highmem.h>

#include "page_hash.h"

/* Proc dir and proc entry to be added */
static struct proc_dir_entry *proc_dir, *proc_entry;

/* Kernel Thread */
static struct task_struct *cs598_kernel_thread;

/* Wait queue for kernel thread to wait on */
static DECLARE_WAIT_QUEUE_HEAD (cs598_waitqueue);

static int cs598_kernel_thread_fn(void *unused)
{
  unsigned long curr_pfn;
  struct page *page;
  void *data;

	/* Declare a waitqueue */
	DECLARE_WAITQUEUE(wait,current);

	/* Add wait queue to the head */
	add_wait_queue(&cs598_waitqueue,&wait);

	while (1) {
		/* Set current state to interruptible */
		set_current_state(TASK_INTERRUPTIBLE);

		/* give up the control */
		schedule();

		/* coming back to running state, check if it needs to stop */
		if (kthread_should_stop()) {
			printk(KERN_INFO "cs598: thread needs to stop\n");
			break;
		}

		printk(KERN_INFO "cs598: hash invoked by procfs\n");

    /* Loop over all of phys memory */
		printk(KERN_INFO "cs598: number of physical pages %u\n", num_physpages);
    for(curr_pfn=1;curr_pfn<num_physpages;curr_pfn++) {
      page = pfn_to_page(curr_pfn);  
      data = kmap(page);
      if(!data) {
        printk(KERN_ALERT "cs598: couldn't map page with pfn %u\n", curr_pfn);
      }
  
      //FIXME: calculate hash here 
      kunmap(data);
    }

		printk(KERN_INFO "cs598: Finished computing hashes\n");
	}

	/* exiting thread, set it to running state */
	set_current_state(TASK_RUNNING);

	/* remove the waitqueue */
	remove_wait_queue(&cs598_waitqueue, &wait);

	printk(KERN_INFO "cs598: thread killed\n");

	return 0;
}

static int cs598_write_proc(struct file *filp,
			    const char __user *buff,
			    unsigned long len,
			    void *data)
{
	wake_up_interruptible(&cs598_waitqueue);
	return len;
}

static int __init cs598_init_module()
{
	int ret = 0;

	/* Create a proc directory entry cs598 */
	proc_dir = proc_mkdir("cs598",NULL);
	if ( proc_dir == NULL )
		goto bad;

	/* Create an entry hash under proc dir cs598 */
	proc_entry = create_proc_entry( "hash", 0666, proc_dir );
	if ( proc_entry == NULL )
		goto bad;

	proc_entry->write_proc = cs598_write_proc;

	/* Create a kernel thread that calculates the hash of phy mem */
	cs598_kernel_thread = kthread_run(cs598_kernel_thread_fn,
					  NULL,
					  "cs598kt");
	if ( cs598_kernel_thread == NULL )
		goto bad;

	printk(KERN_INFO "cs598: Kernel module loaded\n");

	goto end;
 bad:
	if ( proc_entry ) {
		remove_proc_entry("hash",proc_dir);
	}
	if ( proc_dir ) {
		remove_proc_entry("cs598",NULL);
	}
	printk(KERN_INFO "cs598: Error");
	ret = -ENOMEM;
 end:
	return ret;
}

static void __exit cs598_exit_module(void)
{
	/* Remove the proc entries */
	remove_proc_entry("hash",proc_dir);
	remove_proc_entry("cs598",NULL);

	/* Before stopping the thread, put it into running state */
	wake_up_interruptible(&cs598_waitqueue);

	/* now stop the thread */
	kthread_stop(cs598_kernel_thread);

	printk(KERN_INFO "cs598: Kernel module removed\n");
}

module_init(cs598_init_module);
module_exit(cs598_exit_module);

MODULE_LICENSE("GPL");
