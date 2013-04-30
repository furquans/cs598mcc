#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VM 5
#define HASH_SIZE 160

struct node {
  struct node *left;
  struct node *right;
  char key[HASH_SIZE];
  int count[MAX_VM];
}di;

int curr_vm = 0;
int total_count = 0,single_copy = 0;
int total_pages[MAX_VM];

struct node *allocate_new_node(char *str)
{
  struct node *temp = malloc(sizeof(*temp));
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

#define STRING_SIZE 1000000000ULL
#define MAX_VM_NAME 100

void print_stats(int num_vms,
		 int *vm_ids,
		 struct node *root)
{
  int count = 0;
  int i = 0;
  int flag = 0;

  if (root == NULL)
    return;

  for (i = 0 ; i < num_vms ; i++) {
    int val = root->count[vm_ids[i]];
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
  print_stats(num_vms,vm_ids,root->left);
  print_stats(num_vms,vm_ids,root->right);
}

void analyze_vms(struct node *root)
{
  int num_vms;
  int i;
  int *vm_ids;
  int actual_pages = 0; 

  do {
    printf("Enter the number of VMs to consider: ");
    scanf("%d",&num_vms);
  } while ((num_vms > curr_vm) || (num_vms < 2));

  vm_ids = malloc(sizeof(*vm_ids) * num_vms);

  for (i = 0; i < num_vms; i++) {
    printf("Enter id of a VM: ");
    scanf("%d",&vm_ids[i]);
    actual_pages += total_pages[vm_ids[i]];
  }

  total_count = 0;
  single_copy = 0;
  print_stats(num_vms, vm_ids, root);
  printf("Total pages present in duplicates:%d\n",total_count);
  printf("Unique pages in duplicates:%d\n", single_copy);
  printf("Out of actual %d pages occupied in memory,\n",actual_pages);
  printf("after deduplication, total pages occupied would be %d - %d + %d = %d\n",actual_pages,
	 total_count, 
	 single_copy,
	 actual_pages-total_count+single_copy);
}


int main(int argc,
	 char **argv)
{
  struct node *root = NULL;
  int count;
  char vmname[MAX_VM][MAX_VM_NAME];
  char *str;
  const char *delim = " \n";
  int i = 2;

  str = malloc(STRING_SIZE);

  if (argc < 3) {
    printf("Usage: %s count <filename...>\n",argv[0]);
    exit(1);
  }

  count = atoi(argv[1]);
  
  while (count) {
    FILE *fp = fopen(argv[2], "r");
    char *temp;
    while (fgets(str,STRING_SIZE,fp)) {
      temp = strtok(str,delim);
      total_pages[curr_vm] = 0;
      strcpy(vmname[curr_vm],temp);
      printf("Name is %s.\n",vmname[curr_vm]);
      while (temp = strtok(NULL, delim)) {
	printf("Token: %s.\n",temp);
	total_pages[curr_vm]++;
	insert_BST(&root,temp);
      }
      curr_vm++;
    }
    fclose(fp);
    count--;
  }

  printf("::::::::::::::::::Analysis::::::::::::::::::::::::\n");
  do {
    int choice;
    int i = 0;
    printf("::::::::::::::::::Menu::::::::::::::::::::::::::::\n");
    printf("1. Display VM Name\n");
    printf("2. VM analysis\n");
    printf("3. Exit\n");

    printf("Enter a choice: ");
    scanf("%d",&choice);

    switch (choice) {
    case 1:
      printf("Total VMs: %d\n",curr_vm);
      for (i=0 ; i < curr_vm; i++) {
	printf("#%d Name %s Total pages %d\n",i,vmname[i],total_pages[i]);
      }
      break;
    case 2:
      analyze_vms(root);
      break;
    case 3:
      exit(0);
    default:
      printf("error\n");
    }
    
  }while (1);
}
