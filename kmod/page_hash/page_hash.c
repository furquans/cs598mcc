// Copyright 2013 Zachary Estrada and Furquan Shaikh

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/ioctl.h>
#include <linux/mm.h>

#include "page_hash.h"

static struct file_operations ph_fops = {
};

int init_module() {
	int ret;

	ret = register_chrdev(DEV_MAJOR_NUM, DEV_NAME, &ph_fops);
	if (ret < 0)
		printk(KERN_ERR "register_chrdev failed: %d\n", ret);

	return EXIT_SUCCESS;
}

void cleanup_module(void) {
	// Unregister device
	unregister_chrdev(DEV_MAJOR_NUM, DEV_NAME);
}
