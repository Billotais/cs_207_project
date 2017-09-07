
#include <stdint.h>
#include <inttypes.h>
#include "bmblock.h"
#include <stdlib.h>
#include "error.h"
#define BITS 64


struct bmblock_array *bm_alloc(uint64_t min, uint64_t max) {
  // Check parameters
  if (min > max) return NULL;
  uint64_t length = max - min + 1;
  //  How manay more entries in the array we have to add
  size_t toAdd = (length - 1)/ 64;

  struct bmblock_array* ba = malloc(sizeof(struct bmblock_array) + toAdd*BITS);
  if (ba != NULL){
    // If the allocation worked, we update the parameters
    ba->min = min;
    ba->max = max;
    ba->length = toAdd+1;
    ba->cursor = min;
  }
  return ba;
}
int bm_get(struct bmblock_array *bmblock_array, uint64_t x) {
  M_REQUIRE_NON_NULL(bmblock_array);

  // Check parameters
  if (x < bmblock_array->min || bmblock_array->max < x) return ERR_BAD_PARAMETER;

  // Which row 
  size_t bm_index = (x - bmblock_array->min) / BITS;
  // Where in the row
  size_t shift = ((x - bmblock_array->min) % BITS);

  // Get the row
  uint64_t correctUInt = bmblock_array->bm[bm_index];
  // get the value at the positon
  return (int)((correctUInt >> shift) & UINT64_C(1));
}
void bm_set(struct bmblock_array *bmblock_array, uint64_t x) {
  // Check parameters
  if (x < bmblock_array->min || bmblock_array->max < x) return;
  // Which row 
  size_t bm_index = (x - bmblock_array->min) / BITS;
  // Where in the row
  size_t shift = (x - bmblock_array->min) % BITS;
  // set the value at the positon
  bmblock_array->bm[bm_index] = bmblock_array->bm[bm_index] | (UINT64_C(1) << shift);
}
void bm_clear(struct bmblock_array *bmblock_array, uint64_t x) {
  // Check parameters
  if (x < bmblock_array->min || bmblock_array->max < x) return;
  // Which row 
  size_t bm_index = (x - bmblock_array->min) / BITS;
  // Where in the row
  size_t shift = (x - bmblock_array->min) % BITS;
  // clear the value at the positon
  bmblock_array->bm[bm_index] = bmblock_array->bm[bm_index] & ~(UINT64_C(1) << shift);
  // Move the cursor back if needed
  if(bmblock_array->cursor > x) bmblock_array->cursor = x;
}

void bm_print(struct bmblock_array *bmblock_array){
  printf("**********BitMap Block START**********\n");
  printf("length: %" PRIu64 "\n", bmblock_array->length);
  printf("min: %" PRIu64 "\n", bmblock_array->min);
  printf("max: %" PRIu64 "\n", bmblock_array->max);
  printf("cursor: %" PRIu64 "\n", bmblock_array->cursor);
  printf("content:\n");
  // For each row
  for (int i = 0; i < bmblock_array->length; ++i) {
    printf("%d:", i);
    uint64_t row = bmblock_array->bm[i];
    // For each bit in the row
    for (int j = 0; j < BITS; ++ j){
      if (j % 8 == 0) printf(" "); // 8 hard coded, just to show the bits in a nice way. Doesn't really correspond to given constants
      printf("%" PRIu64 "", (row >> j) & UINT64_C(1));
    }
    printf("\n");
  }
  printf("**********BitMap Block END************\n");

}

int bm_find_next(struct bmblock_array *bmblock_array) {
  M_REQUIRE_NON_NULL(bmblock_array);
  // Our cursor correspond to a bit and not a row (It was said in the forum taht since it was an intertnal representation it was ok)
  while (1) {
    // get the row
    uint64_t bm_index = bmblock_array->bm[(bmblock_array->cursor - bmblock_array->min) / BITS];
    // If every bit of the row is assigned
    if (bm_index == UINT64_C(-1)){
      // If we are at the end of the bm_block, return err_full
      if((bmblock_array->cursor - bmblock_array->min) / BITS + 1 == bmblock_array->length){
        return ERR_BITMAP_FULL;
      }
      // Else got to the next row
      bmblock_array->cursor += BITS;
	  } else break; // If the row is not full, got to the next part to find which bit is available
  }

  uint8_t curr_bit = 1;

  do {
    // Get the bit at the cursor position and move by 1
    curr_bit = bm_get(bmblock_array, bmblock_array->cursor);
    (bmblock_array->cursor)++;
  } while(curr_bit != 0 && bmblock_array->cursor <= bmblock_array->max); // Until the bit = 0 or we are out of range (We normaly should never be out of range but we check anyways in case of a problem)
  // Return either the bit we found or an error if we went out of range
  return (curr_bit == 0) ? --(bmblock_array->cursor) : ERR_BITMAP_FULL; // --cursor because we moved by one when we read a bit
}
