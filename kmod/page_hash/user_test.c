#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define PAGE_SIZE getpagesize()

//Loosely based on http://memset.wordpress.com/2010/10/06/using-sha1-function/
void print_sha1(char *hash, const char *str);

int main(int argn, char *argv[]) {
	unsigned char *page, *hash; 
    int fd;
    unsigned char temp[SHA_DIGEST_LENGTH];
    memset(temp, 0x0, SHA_DIGEST_LENGTH);

    //Bring in our page to test
    fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0) {
        perror("Couldn't open /dev/urandom");
        abort();
    }
    page = mmap(0, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON,
                -1, 0);
    if(page == MAP_FAILED) { 
        fprintf(stderr, "Couldn't mmap %d bytes!\n", PAGE_SIZE);
        perror("errno");
        abort();
    }
 
    //First, get a zero page
    printf("Got page at 0x%p\n", (void *)page);

    hash = SHA1(page, PAGE_SIZE, temp);
    if(hash != temp) {
        fprintf(stderr, "SHA1 failed!");
        abort();
    }
    print_sha1(hash,"Zero page:");
    //Our random page
    read(fd, page, PAGE_SIZE);
    hash = SHA1(page, PAGE_SIZE, temp);

    print_sha1(hash,"Randomly generated page:");
    printf("Pausing with page in memory...\n");
    pause();
	munmap(page, PAGE_SIZE);
    return 0;
}

void print_sha1(char *hash, const char *str) {
    char buf[(SHA_DIGEST_LENGTH*2)+1];
    int i;
 
    memset(buf, 0x0, (SHA_DIGEST_LENGTH*2)+1);
    for (i=0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf((char*)&(buf[i*2]), "%02x", hash[i]);
    }
 
    printf("%s %s\n", str, buf);
}
