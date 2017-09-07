#include "filev6.h"
#include <stdio.h>
#include "unixv6fs.h"
#include "inode.h"
#include <string.h>
#include <openssl/sha.h>
#include "sha.h"

int test(struct unix_filesystem *u) {
  struct filev6 stv6;
  memset(&stv6, 255, sizeof(stv6));

  int inodeOpen = filev6_open(u, 3, &stv6);
  if(inodeOpen < 0) {printf("filev6_open failed for inode #%d.\n", 3);}
  else {
    printf("\nPrinting inode #3: \n");
    inode_print(&(stv6.i_node));
    if (((stv6.i_node).i_mode & IFDIR)) {printf("C'est un repertoire\n");}
    else {
      unsigned char data[SECTOR_SIZE + 1];
      data[SECTOR_SIZE] = '\0';
      int fileRead = filev6_readblock(&stv6, &data);
      if(fileRead < 0) {printf("Error while reading file");}
      printf("the first sector of data of which contains:\n%s\n", data);

      printf("----\n");
    }
  }

  int inodeOpen2 = filev6_open(u, 5, &stv6);
  if(inodeOpen2 < 0) {printf("filev6_open failed for inode #%d.\n", 5);}
  else {
    printf("\nPrinting inode #5: \n");
    inode_print(&(stv6.i_node));
    if (((stv6.i_node).i_mode & IFDIR)) {printf("which is a directory");}
    else {
      unsigned char data[SECTOR_SIZE + 1];
      data[SECTOR_SIZE] = '\0';
      int fileRead = filev6_readblock(&stv6, &data);
      if(fileRead < 0) {printf("Error while reading file");}
      printf("the first sector of data of which contains:\n%s\n", data);

      printf("----\n");


    }

  }
  int openedInode;
  int i = 1;
  while((openedInode = filev6_open(u, i, &stv6)) == 0)
  {
    print_sha_inode(u, stv6.i_node, i);
    ++i;
  }
  return 0;
}
