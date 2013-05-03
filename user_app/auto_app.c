#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VM 5
#define HASH_SIZE 25
#define STRING_SIZE 1000000000ULL
#define MAX_VM_NAME 100

struct node {
  struct node *left;
  struct node *right;
  char key[HASH_SIZE];
  int count[MAX_VM];
}di;

int curr_vm = 0, num_vms;
int total_count = 0,single_copy = 0;
int total_pages[MAX_VM];
char vmname[MAX_VM][MAX_VM_NAME];

struct node *allocate_new_node(char *str)
{
  struct node *temp = malloc(sizeof(*temp));
  if (temp == NULL) {
    printf("temp allocation failed\n");
    exit(1);
  }
  strcpy(temp->key,str);
  memset(temp->count, 0, MAX_VM * sizeof(int));
  temp->count[curr_vm] = 1;
  temp->left = temp->right = NULL;
}

void insert_BST(struct node **root,
		char *hash_val)
{
  struct node *new;
  int ret = 0;

  if ( *root == NULL ) {
    new = allocate_new_node(hash_val);
    *root = new;
  } else {
    struct node *parent,*temp = *root;

    while (temp != NULL) {
      parent = temp;
      ret = strcmp(hash_val,temp->key);
      if (ret == 0) {
	break;
      } else if (ret < 0) {
	temp = temp->left;
      } else {
	temp = temp->right;
      }
    }

    if ( temp ) {
      temp->count[curr_vm]++;
    } else {
      new = allocate_new_node(hash_val);
      if (ret < 0) {
	parent->left = new;
      } else {
	parent->right = new;
      }
    }
  }
}

void print_stats(struct node *root)
{
  int count = 0;
  int i = 0;
  int flag = 0;

  if (root == NULL)
    return;

  for (i = 0 ; i < num_vms ; i++) {
    int val = root->count[i];
    if (val == 0) {
      flag = 1;
      break;
    }
    count += val;
  }
  if (flag == 0) {
    single_copy++;
    total_count += count;
  }
  print_stats(root->left);
  print_stats(root->right);
}

void analyze_vms(struct node *root)
{
  int i;
  int actual_pages = 0; 

  for (i = 0; i < num_vms; i++) {
    actual_pages += total_pages[i];
  }

  total_count = 0;
  single_copy = 0;
  print_stats(root);
  printf("Total pages present in duplicates:%d\n",total_count);
  printf("Unique pages in duplicates:%d\n", single_copy);
  printf("Out of actual %d pages occupied in memory,\n",actual_pages);
  printf("after deduplication, total pages occupied would be %d - %d + %d = %d\n",actual_pages,
	 total_count, 
	 single_copy,
	 actual_pages-total_count+single_copy);
  printf("Total pages present in duplicates:%d\n",total_count);
  printf("Percentage of pages that are duplicates:%g\n",
         ((double)total_count/(double)actual_pages)*100);
}

int check_vm_list(char *str)
{
  int i = 0;

  for (; i<num_vms; i++) {
    if (strcmp(vmname[i],str) == 0)
      return i;
  }
  return -1;
}

int main(int argc,
	 char **argv)
{
  struct node *root = NULL;
  int count;
  char *str;
  const char *delim = " \n";
  int i = 2;

  str = malloc(STRING_SIZE);

  if (str == NULL) {
    printf("str allocation failed\n");
    exit(1);
  }

  if (argc < 3) {
    printf("Usage: %s filename VMNAME1 VMNAME2 <VMNAME3...>\n",argv[0]);
    exit(1);
  }

  num_vms = count = argc-2;

  while (count) {
    strcpy(vmname[i-2],argv[i]);
    count--;
    printf("vmname:%s\n",vmname[i-2]);
    i++;
  }
  
  FILE *fp = fopen(argv[1], "r");

  while (fgets(str,STRING_SIZE,fp)) {
    char *temp;
    temp = strtok(str,delim);
    curr_vm = check_vm_list(temp);
    if (curr_vm == -1)
      continue;
    total_pages[curr_vm] = 0;
    while (temp = strtok(NULL, delim)) {
      /* printf("Token: %s.\n",temp); */
      total_pages[curr_vm]++;
      insert_BST(&root,temp);
    }
  }
  fclose(fp);
  analyze_vms(root);
}
