#include "bmblock.h"
#include "direntv6.h"
#include "filev6.h"
#include "error.h"
#include "unixv6fs.h"
#include "inode.h"
#include <stdio.h>
#include "mount.h"


int main(void){
  struct bmblock_array *bm = bm_alloc(4, 131);

  bm_print(bm);
  printf("find_next() = %d\n", bm_find_next(bm));
  bm_set(bm, 4);
  bm_set(bm, 5);
  bm_set(bm, 6);
  bm_print(bm);
  printf("find_next() = %d\n", bm_find_next(bm));
  for (int i = 4; i <= 130; i += 3){
    bm_set(bm, i);
  }
  bm_print(bm);
  printf("find_next() = %d\n", bm_find_next(bm));
  for (int j = 5; j <= 130; j += 5){
    bm_clear(bm, j);
  }
  bm_print(bm);
  printf("find_next() = %d\n", bm_find_next(bm));

}

