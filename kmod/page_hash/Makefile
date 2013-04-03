KVERSION = $(shell uname -r)
MODNAME=page_hash
MAJNUM=101
EXTRA_CFLAGS="-O0"
obj-m = $(MODNAME).o
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
	gcc -o $(MODNAME)_user $(MODNAME)_user.c

install:
	if test -e /dev/$(MODNAME); then rm -f /dev/$(MODNAME); /sbin/rmmod $(MODNAME); fi
	/bin/mknod /dev/$(MODNAME) c $(MAJNUM) 0
	/sbin/insmod $(MODNAME).ko

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
	rm -f $(MODNAME)_user
