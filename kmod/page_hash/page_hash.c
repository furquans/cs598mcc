// Copyright 2013 Zachary Estrada and Furquan Shaikh

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/ioctl.h>
#include <linux/mm.h>

#include "page_hash.h"

/* Proc dir and proc entry to be added */
static struct proc_dir_entry *proc_dir, *proc_entry;

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

	printk(KERN_INFO "cs598: Kernel module loaded\n");

	goto end;
 bad:
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

	printk(KERN_INFO "cs598: Kernel module removed\n");
}

module_init(cs598_init_module);
module_exit(cs598_exit_module);

MODULE_LICENSE("GPL");
