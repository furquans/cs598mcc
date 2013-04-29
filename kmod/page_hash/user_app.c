#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>

#define NPAGES sysconf(_SC_PHYS_PAGES)
//#define NPAGES 100
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

static int buf_fd = -1;
static int buf_len;

#define HASH_SIZE 20

void *buf_init(char *fname)
{
  unsigned int *kadr;

  buf_len = NPAGES * HASH_SIZE;
  if ((buf_fd = open(fname, O_RDWR|O_SYNC)) < 0) {
    printf("file open error %s\n",fname);
    return NULL;
  }

  kadr = mmap(0,
	      buf_len,
	      PROT_READ|PROT_WRITE,
	      MAP_SHARED,
	      buf_fd,
	      0);
  if (kadr == MAP_FAILED) {
    perror("buf file open error");
    return NULL;
  }
  return kadr;
}

void buf_exit()
{
  close(buf_fd);
}

static void hash_to_str(char *hash, char *buf) {
  int i;  
  for(i=0;i<HASH_SIZE;i++) {
    sprintf((char*)&(buf[i*2]), "%02x", hash[i]);
  }
}


int main(int argc, char **argv)
{
  char *buf;
  int index = 0;
  char *str;
  const char *vmname;

  if (argc < 2) {
    fprintf(stderr, "usage: %s VMNAME\n", argv[0]);
    return EINVAL;
  } 

  vmname = argv[1];

  buf = buf_init("node");
  if (!buf)
    return -1;

  str = malloc((2*HASH_SIZE)+1);
  printf("%s ", vmname); 
  while (index <= NPAGES) {
    hash_to_str(buf+(index*HASH_SIZE),str);
    //The index e.g. 40 corresponds to the 41st char
    str[(2*HASH_SIZE)] = '\0';
    printf("%s ",str);
    index++;
  } 
  printf("\n"); 
  free(str);

  buf_exit();
}
