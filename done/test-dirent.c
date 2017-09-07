#include "direntv6.h"
#include "filev6.h"
#include "error.h"
#include "unixv6fs.h"
#include "inode.h"
#include <stdio.h>


int test(struct unix_filesystem *u) {
  return direntv6_print_tree(u, ROOT_INUMBER, "");
}
