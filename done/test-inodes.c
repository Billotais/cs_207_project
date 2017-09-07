#include "inode.h"
#include "unixv6fs.h"
#include "filev6.h"
#include "bmblock.h"
#include "mount.h"
int test(struct unix_filesystem *u) {

  bm_print(u->ibm);
  struct inode i_node;
  
  struct filev6 fv6 = {u, 12, i_node, 0};

  
  int tryCreate = filev6_create(u, IALLOC, &fv6);
  if (tryCreate != 0) return tryCreate;
  bm_print(u->ibm);
  inode_scan_print(u);
  return 0;
}
