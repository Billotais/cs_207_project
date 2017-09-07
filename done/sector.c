
#include <stdio.h>
#include "unixv6fs.h"
#include "error.h"

// Implemented WEEK 4
/**
 * @brief read one 512-byte sector from the virtual disk
 * @param f open file of the virtual disk
 * @param sector the location (in sector units, not bytes) within the virtual disk
 * @param data a pointer to 512-bytes of memory (OUT)
 * @return 0 on success; <0 on error
 */
int sector_read(FILE *f, uint32_t sector, void *data) {
  M_REQUIRE_NON_NULL(f);
  M_REQUIRE_NON_NULL(data);

  // Move the cursor at the correct position in the file
  int cursor = fseek(f, sector*SECTOR_SIZE, SEEK_SET);
  if (cursor != 0) return ERR_IO;

  // Read one sector starting at the position of the cursor
  int read = fread(data, SECTOR_SIZE, 1, f);
  if (read != 1) return ERR_IO;

  return 0;
}


// Implemented WEEK 11
/**
 * @brief read one 512-byte sector from the virtual disk
 * @param f open file of the virtual disk
 * @param sector the location (in sector units, not bytes) within the virtual disk
 * @param data a pointer to 512-bytes of memory (IN)
 * @return 0 on success; <0 on error
 */
int sector_write(FILE *f, uint32_t sector, void *data){
  M_REQUIRE_NON_NULL(f);
  M_REQUIRE_NON_NULL(data);
  
  // Move the cursor at the correct position
  int cursor = fseek(f, sector*SECTOR_SIZE, SEEK_SET);
  if (cursor != 0) return ERR_IO;

  // Write the sector
  int write = fwrite(data, SECTOR_SIZE, 1, f);
  if (write != 1) return ERR_IO;
	
 

  return 0;
}
