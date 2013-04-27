obj-m += page_hash.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o user_app user_app.c
	gcc -o user_test user_test.c -lcrypto

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf user_app

load:
	./load_module.sh

