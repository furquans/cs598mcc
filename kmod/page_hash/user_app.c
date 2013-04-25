#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>

#define NPAGES 100
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
  int index = 1;
  char *str;

  buf = buf_init("node");
  if (!buf)
    return -1;

  str = malloc((2*HASH_SIZE)+1);
  str[(2*HASH_SIZE)+1] = '\0';
  while (index <= 500) {
    hash_to_str(buf+(index*HASH_SIZE),str);
    printf("Hash %d:%s\n",index,str);
    index++;
  } 
  free(str);

  buf_exit();
}
