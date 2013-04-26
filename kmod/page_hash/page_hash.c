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
#include <linux/crypto.h>
#include <crypto/hash.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/scatterlist.h>

#include "page_hash.h"

/* hash parameters */
#define HASH_SIZE 20
#define HASH_ALG "sha1"

/* Number of pages to store hash -- considering 512MB of memory to hash*/
#define NPAGES num_physpages
//#define NPAGES 500

/* Proc dir and proc entry to be added */
static struct proc_dir_entry *proc_dir, *proc_entry;

/* Kernel Thread */
static struct task_struct *cs598_kernel_thread;

/* Wait queue for kernel thread to wait on */
static DECLARE_WAIT_QUEUE_HEAD (cs598_waitqueue);

/* Buffer to be shared with user space process */
static u8 *vmalloc_buffer;

/* Character device specific */
static int cs598_dev_major, cs598_dev_minor = 0;
static int cs598_nr_devs = 1;
static dev_t cs598_dev;
static struct cdev *cs598_cdev;

int cs598_dev_mmap(struct file *, struct vm_area_struct *);

static struct file_operations cs598_dev_fops = {
	.owner = THIS_MODULE,
	.open = NULL,
	.release = NULL,
	.mmap = cs598_dev_mmap,
};

/* Hash done flag */
static int hash_done_flag = 0;

static int cs598_kernel_thread_fn(void *unused)
{
	unsigned long curr_pfn;
	struct page *page;
	void *data, *pagebuf;
	struct crypto_hash *tfm;
	struct scatterlist sg;
	struct hash_desc desc; //Hopefully this can go on the stack
	int err;

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
		printk(KERN_INFO "cs598: number of physical pages %lu\n", num_physpages);
		printk(KERN_INFO "cs598: number of pages that will be hashed %lu\n", NPAGES);

		for(curr_pfn=1;curr_pfn<NPAGES;curr_pfn++) {
			page = pfn_to_page(curr_pfn);  
			data = kmap(page);
			if(!data) {
				printk(KERN_ALERT "cs598: couldn't map page with pfn %lu\n", curr_pfn);
				break;
			}
			tfm = crypto_alloc_hash(HASH_ALG, 0, CRYPTO_ALG_ASYNC);
			if(tfm == NULL) {
				printk(KERN_ALERT "cs598: got NULL tfm \n");
				kunmap(data);
				break;
			}
			desc.tfm = tfm;
			desc.flags = 0;
			if(curr_pfn == 1)
				printk(KERN_INFO "cs598: hash size %d\n", crypto_hash_digestsize(tfm));
			if(crypto_hash_digestsize(tfm) > HASH_SIZE) {
				printk(KERN_ALERT "cs598: hash alg report size of %d, but HASH_SIZE = %d\n", crypto_hash_digestsize(tfm),HASH_SIZE);
				goto done_error;
			}
			//pagebuf = kmalloc(PAGE_SIZE, GFP_USER);
			//memcpy(pagebuf, data, PAGE_SIZE);
			sg_init_one(&sg, data, PAGE_SIZE);
			crypto_hash_init(&desc);	
			err = crypto_hash_update(&desc, &sg, PAGE_SIZE);
			if(err) {
				printk(KERN_ALERT "cs598: crypto_shash_update returned: %d\n",
						err);
				goto done_error;
			}
            err = crypto_hash_final(&desc, vmalloc_buffer+(curr_pfn*HASH_SIZE));
			if(err) {
				printk(KERN_ALERT "cs598: crypto_shash_digest returned: %d\n",
						err);
				goto done_error;
			}
			kunmap(data);
			crypto_free_hash(tfm);
            //Don't bring everything to an absolute halt
            if(curr_pfn % 10000)
                schedule();
		}
		hash_done_flag = 1;
		printk(KERN_INFO "cs598: Finished computing hashes\n");
	}
	
	/* exiting thread, set it to running state */
	set_current_state(TASK_RUNNING);
	
	/* remove the waitqueue */
	remove_wait_queue(&cs598_waitqueue, &wait);

	printk(KERN_INFO "cs598: thread killed\n");

	return 0;
	done_error:
		kunmap(data);	
		crypto_free_hash(tfm);
		printk(KERN_INFO "cs598: aborted computing hashes\n");
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&cs598_waitqueue, &wait);
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

static int cs598_read_proc(char *page, char **start, off_t off,
			   int count, int *eof, void *data)
{
	return sprintf(page, "%d", hash_done_flag);
}

static int allocate_buffer(void)
{
	int i;

	if ((vmalloc_buffer = vzalloc(NPAGES * HASH_SIZE)) == NULL) {
		return -ENOMEM;
	}

	/* Set PG_RESERVED bit of pages to avoid MMU from swapping out the pages */
	/* Done for every page */
	for (i = 0; i < NPAGES; i++) {
		SetPageReserved(vmalloc_to_page((void*)(((unsigned long)vmalloc_buffer)
							+ (i*HASH_SIZE))));
	}

	return 0;
}

static void free_buffer(void)
{
	int i;

	/* Clear the PG_RESERVED bits of the pages */
	for (i = 0;i < NPAGES;i++ ) {
		ClearPageReserved(vmalloc_to_page((void*)(((unsigned long)vmalloc_buffer)
							  + (i*HASH_SIZE))));
	}

	vfree(vmalloc_buffer);
}

static int cs598_create_char_dev(void)
{
	int result = 0;

	/* Dynamically allocate a major number for the device */
	result = alloc_chrdev_region(&cs598_dev,
				     cs598_dev_minor,
				     cs598_nr_devs,
				     "my_char_dev");
	cs598_dev_major = result;
	if (result < 0) {
		printk(KERN_INFO "cs598: Char dev cannot get major\n");
		return result;
	}

	/* Allocate a character device structure */
	cs598_cdev = cdev_alloc();

	/* Assign the function pointers */
	cs598_cdev->ops = &cs598_dev_fops;
	cs598_cdev->owner = THIS_MODULE;

	/* Add this character device */
	result = cdev_add(cs598_cdev, cs598_dev, 1);
	if (result) {
		printk(KERN_INFO "cs598: Error adding device\n");
		return result;
	}
	return result;
}

int cs598_dev_mmap(struct file *fp, struct vm_area_struct *vma)
{
	int ret,i;
	unsigned long length = vma->vm_end - vma->vm_start;

	if (length > (NPAGES * HASH_SIZE)) {
		return -EIO;
	}

	/* Done for every page */
	for (i=0; i < length; i+=PAGE_SIZE) {
		/* Remap every page in the virtual address space of the user process.
		   This is required so that the process can access with correct privilege
		   Else MMU will report violation */
		if ((ret = remap_pfn_range(vma,
					   vma->vm_start + i,
					   /* Convert virtual address to page frame number */
					   vmalloc_to_pfn((void*)(((unsigned long)vmalloc_buffer)
								  + i)),
					   PAGE_SIZE,
					   vma->vm_page_prot)) < 0) {
			printk(KERN_INFO "cs598:mmap failed");
			return ret;
		}
	}
	printk(KERN_INFO "cs598:mmap successful");
	return 0;
}

static void cs598_destroy_char_dev(void)
{
	/* Delete the character device */
	cdev_del(cs598_cdev);

	/* Unregister the character device */
	unregister_chrdev_region(cs598_dev, cs598_nr_devs);
}

static int __init cs598_init_module(void)
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
	proc_entry->read_proc = cs598_read_proc;

	/* Create a kernel thread that calculates the hash of phy mem */
	cs598_kernel_thread = kthread_run(cs598_kernel_thread_fn,
					  NULL,
					  "cs598kt");
	if ( cs598_kernel_thread == NULL )
		goto bad;

	/* Allocate buffer to share with user app */
	if (allocate_buffer() == -ENOMEM) {
		ret = -ENOMEM;
		goto bad;
	}

	/* Create a character device */
	if ((ret = cs598_create_char_dev()) != 0) {
		goto bad;
	}

	printk(KERN_INFO "cs598: Kernel module loaded\n");

	goto end;
 bad:
	if ( vmalloc_buffer ) {
		free_buffer();
	}

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

	/* Free allocated buffer */
	free_buffer();

	/* Destroy character device */
	cs598_destroy_char_dev();

	printk(KERN_INFO "cs598: Kernel module removed\n");
}


module_init(cs598_init_module);
module_exit(cs598_exit_module);

MODULE_LICENSE("GPL");
