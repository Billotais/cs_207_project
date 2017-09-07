#include "inode.h"
#include "unixv6fs.h"
#include "filev6.h"
#include "bmblock.h"
int test(struct unix_filesystem *u) {

    //correcteur: side effect : -0.5

  struct inode i_node;
  
  struct filev6 fv6 = {u, 6, i_node, 0};

  
  int tryCreate = filev6_create(u,IFDIR,&fv6);
  if (tryCreate != 0) return tryCreate;
  bm_print(u->ibm);
  inode_scan_print(u);
  return 0;
}
