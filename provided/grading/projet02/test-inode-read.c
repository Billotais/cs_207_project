#include "inode.h"
int test(struct unix_filesystem *u) {
  struct inode readInode;
  inode_read(u, 5, &readInode);
  inode_print(&readInode);
  printf("%d\n",inode_findsector(u, &readInode, 8));
  return 0;
}
