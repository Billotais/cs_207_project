
#include <stdint.h>
#include <inttypes.h>
#include "bmblock.h"
#include <stdlib.h>
#include "error.h"
#define BITS 64


struct bmblock_array *bm_alloc(uint64_t min, uint64_t max) {
  if (min > max) return NULL;
  uint64_t length = max - min + 1;
  uint64_t toAdd = (length - 1)/ 64;
  struct bmblock_array* ba = malloc(sizeof(struct bmblock_array) + toAdd*BITS);
  if (ba != NULL){
    ba->min = min;
    ba->max = max;
    ba->length = toAdd+1;
    ba->cursor = min;
  }
  return ba;
}
int bm_get(struct bmblock_array *bmblock_array, uint64_t x) {

  if (x < bmblock_array->min || bmblock_array->max < x) return ERR_BAD_PARAMETER;

  size_t bm_index = (x - bmblock_array->min) / BITS;
  size_t shift = ((x - bmblock_array->min) % BITS);

  uint64_t correctUInt = bmblock_array->bm[bm_index];
  return (int)((correctUInt >> shift) & UINT64_C(1));
}
void bm_set(struct bmblock_array *bmblock_array, uint64_t x) {
  if (x < bmblock_array->min || bmblock_array->max < x) return;
  size_t bm_index = (x - bmblock_array->min) / BITS;
  size_t shift = (x - bmblock_array->min) % BITS;

  bmblock_array->bm[bm_index] = bmblock_array->bm[bm_index] | (UINT64_C(1) << shift);
}
void bm_clear(struct bmblock_array *bmblock_array, uint64_t x) {
  if (x < bmblock_array->min || bmblock_array->max < x) return;
  size_t bm_index = (x - bmblock_array->min) / BITS;
  size_t shift = (x - bmblock_array->min) % BITS;

  bmblock_array->bm[bm_index] = bmblock_array->bm[bm_index] & ~(UINT64_C(1) << shift);
  if(bmblock_array->cursor > x) bmblock_array->cursor = x;
}

void bm_print(struct bmblock_array *bmblock_array){
  printf("**********BitMap Block START**********\n");
  printf("length: %" PRIu64 "\n", bmblock_array->length);
  printf("min: %" PRIu64 "\n", bmblock_array->min);
  printf("max: %" PRIu64 "\n", bmblock_array->max);
  printf("cursor: %" PRIu64 "\n", bmblock_array->cursor);
  printf("content:\n");
  for (int i = 0; i < bmblock_array->length; ++i) {
    printf("%d:", i);
    uint64_t row = bmblock_array->bm[i];


    for (int j = 0; j < 64; ++ j){
      if (j % 8 == 0) printf(" ");
      printf("%" PRIu64 "", (row >> j) & UINT64_C(1));
    }
    printf("\n");
  }
  printf("**********BitMap Block END************\n");

}

int bm_find_next(struct bmblock_array *bmblock_array) {

  do {
    uint64_t bm_index = bmblock_array->bm[(bmblock_array->cursor - bmblock_array->min) % BITS];
    if (bm_index == UINT64_C(-1)){
      if(bm_index + 1 == bmblock_array->length){
        return -1;
      }
      bmblock_array->cursor = (bm_index + 1) * BITS + bmblock_array->min;
    } else break;
  } while (1);

  int curr_bit = 1;

  do {
    curr_bit = bm_get(bmblock_array, bmblock_array->cursor);
    (bmblock_array->cursor)++;
  } while(curr_bit != 0 && bmblock_array->cursor <= bmblock_array->max);
  return (curr_bit == 0) ? --(bmblock_array->cursor) : -1;
}
