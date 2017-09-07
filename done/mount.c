#include <stdio.h>
#include "unixv6fs.h"
#include "bmblock.h"
#include <string.h>
#include "mount.h"
#include "error.h"
#include "sector.h"
#include <inttypes.h>

#include "filev6.h"
#include "inode.h"

void fill_ibm(struct unix_filesystem* ufs);
void fill_fbm(struct unix_filesystem* ufs);
/**
 * @brief  mount a unix v6 filesystem
 * @param filename name of the unixv6 filesystem on the underlying disk (IN)
 * @param u the filesystem (OUT)
 * @return 0 on success; <0 on error
 */
int mountv6(const char *filename, struct unix_filesystem *u) {
  M_REQUIRE_NON_NULL(filename);
  M_REQUIRE_NON_NULL(u);

  // Initialize unix_filesystem struct to 0
  memset(u, 0, sizeof(*u));



  FILE* file = fopen(filename, "r+b");
  if (file == NULL) return ERR_IO;

  u->f = file;


  // Since BOOTBLOCK_MAGIC_NUM is a byte, we use an array of bytes
  uint8_t toBeReadBoot[SECTOR_SIZE];

  // Read BOOTBLOCK_SECTOR, if error, return it
  int bootSector = sector_read(u->f, BOOTBLOCK_SECTOR, &toBeReadBoot);
  if (bootSector != 0) return bootSector;


  // Compare BOOTBLOCK_MAGIC_NUM_OFFSET bytes
  if (toBeReadBoot[BOOTBLOCK_MAGIC_NUM_OFFSET] != BOOTBLOCK_MAGIC_NUM) return ERR_BADBOOTSECTOR;

  // Since the superblock struct is mainly composed of uint16_t, create an array of those
  uint16_t toBeReadSuper[SECTOR_SIZE/2];

  // Read SUPERBLOCK_SECTOR, if error return it
  int superblock = sector_read(file, SUPERBLOCK_SECTOR, &toBeReadSuper);
  if (superblock != 0) return superblock;

  // Set all fields of the superblock
  (u->s).s_isize = toBeReadSuper[0];
  (u->s).s_fsize = toBeReadSuper[1];
  (u->s).s_fbmsize = toBeReadSuper[2];
  (u->s).s_ibmsize = toBeReadSuper[3];
  (u->s).s_inode_start = toBeReadSuper[4];
  (u->s).s_block_start = toBeReadSuper[5];
  (u->s).s_fbm_start = toBeReadSuper[6];
  (u->s).s_ibm_start = toBeReadSuper[7];
  (u->s).s_flock = toBeReadSuper[8] >> 8;
  (u->s).s_ilock = toBeReadSuper[8] << 8 >> 8;
  (u->s).s_fmod = toBeReadSuper[9] >> 8;
  (u->s).s_ronly = toBeReadSuper[9] << 8 >> 8;
  (u->s).s_time[0] = toBeReadSuper[10];
  (u->s).s_time[1] = toBeReadSuper[11];

  // Allocate fbm and ibm

  u->ibm = bm_alloc(2, ((u->s).s_isize - (u->s).s_inode_start + 2)*16 -1);

  u->fbm = bm_alloc((u->s).s_block_start + 1, (u->s).s_fsize - 1);

  // Fill fbm and ibm
  fill_ibm(u);

  fill_fbm(u);

  return 0;
}

/**
 * @brief print to stdout the content of the superblock
 * @param u - the mounted filesytem
 */
void mountv6_print_superblock(const struct unix_filesystem *u) {

  // Print every field of the superblock
  printf("**********FS SUPERBLOCK START**********\n");
  printf("s_isize             : %-4"PRIu16"\n", (u->s).s_isize);
  printf("s_fsize             : %-4"PRIu16"\n", (u->s).s_fsize );
  printf("s_fbmsize           : %-4"PRIu16"\n", (u->s).s_fbmsize);
  printf("s_ibmsize           : %-4"PRIu16"\n", (u->s).s_ibmsize);
  printf("s_inode_start       : %-4"PRIu16"\n", (u->s).s_inode_start);
  printf("s_block_start       : %-4"PRIu16"\n", (u->s).s_block_start);
  printf("s_fbm_start         : %-4"PRIu16"\n", (u->s).s_fbm_start);
  printf("s_ibm_start         : %-4"PRIu16"\n", (u->s).s_ibm_start);
  printf("s_flock             : %-4"PRIu8"\n", (u->s).s_flock);
  printf("s_ilock             : %-4"PRIu8"\n", (u->s).s_ilock);
  printf("s_fmod              : %-4"PRIu8"\n", (u->s).s_fmod);
  printf("s_ronly             : %-4"PRIu8"\n", (u->s).s_ronly);
  printf("s_time              : [%"PRIu8"] %"PRIu8"\n", (u->s).s_time[0], (u->s).s_time[1]);
  printf("**********FS SUPERBLOCK END**********\n");
}

/**
 * @brief umount the given filesystem
 * @param u - the mounted filesytem
 * @return 0 on success; <0 on error
 */
int umountv6(struct unix_filesystem *u) {
  M_REQUIRE_NON_NULL(u);
  M_REQUIRE_NON_NULL(u->f);
  // Try to close
  int closed = fclose(u->f);
  // If error return it
  if (closed != 0) return ERR_IO;
  
  u->f = NULL;

  return 0;
}

/*
 * staff only; students will not have to implement
 */
/**
 * @brief create a new filesystem
 * @param num_blocks the total number of blocks (= max size of disk), in sectors
 * @param num_inodes the total number of inodes
 */
int mountv6_mkfs(const char *filename, uint16_t num_blocks, uint16_t num_inodes){
  M_REQUIRE_NON_NULL(filename);

  // Create the superblock
  struct superblock su;
  memset(&su, 0, sizeof(su));
  su.s_isize = (num_inodes - 1) / INODES_PER_SECTOR + 1;
  su.s_fsize = num_blocks;
  if (su.s_fsize < su.s_isize + num_inodes) return ERR_NOT_ENOUGH_BLOCS;
  su.s_inode_start = SUPERBLOCK_SECTOR + 1;
  su.s_block_start = su.s_inode_start + su.s_isize;

  // open the file
  FILE* f = fopen(filename, "wb");
  if (f == NULL) return ERR_IO;

  // Set the boot sector
  uint8_t bootSector[SECTOR_SIZE];
  memset(bootSector, 0, SECTOR_SIZE);
  bootSector[BOOTBLOCK_MAGIC_NUM_OFFSET] = BOOTBLOCK_MAGIC_NUM;

  // Wrtie bootsector
  int writeBoot = sector_write(f, BOOTBLOCK_SECTOR,bootSector);
  if (writeBoot != 0){fclose(f);return writeBoot;}

  // Wrtie superblock
  int writeSuperBlock = sector_write(f, SUPERBLOCK_SECTOR, &su);
  if (writeSuperBlock != 0) {fclose(f);return writeSuperBlock;}

  // Create the first indoe sector with the root inode
  uint16_t rootInode[ADDRESSES_PER_SECTOR];
  memset(rootInode, 0, SECTOR_SIZE);
  // I_mode of the inode 1
  rootInode[16] = IALLOC | IFDIR;
  
  // Write the root inode sector
  int writeRoot = sector_write(f, su.s_inode_start, rootInode);
  if (writeRoot != 0) {fclose(f);return writeRoot;}

  // Write all othwer inode sectors
  uint8_t emptyInode[SECTOR_SIZE];
  memset(emptyInode, 0, SECTOR_SIZE);

  for (int sect = su.s_inode_start + 1; sect < su.s_block_start; ++sect) {
    int writeEmpty = sector_write(f, sect, emptyInode);
    if (writeEmpty != 0) {fclose(f);return writeEmpty;}
  }

  // Close the file
  fclose(f);
  return 0;
}
void fill_ibm(struct unix_filesystem* ufs) {

  // For each sector containing inodes
  size_t max_i = (ufs->s).s_inode_start + (ufs->s).s_isize;
  for (int i = (ufs->s).s_inode_start; i < max_i; ++i) {

    struct inode toBeRead[INODES_PER_SECTOR];
    // Read the ith sector
    int sector = sector_read(ufs->f, i, toBeRead);
    if (sector != 0) { // If error, we consifer that all inodes are used
      for (int j = 0; j < INODES_PER_SECTOR; ++j) {
        bm_set(ufs->ibm, (uint64_t) ((i-(ufs->s).s_inode_start)*INODES_PER_SECTOR + j));
      }
    }
    else {
      // Go through each inode of the current sector
      for (int j = 0; j < INODES_PER_SECTOR; ++j) { 
        // If allocated fill ibm
        if (toBeRead[j].i_mode & IALLOC) {
          bm_set(ufs->ibm,  ((i-(ufs->s).s_inode_start)*INODES_PER_SECTOR + j));
        }
      }
    }
  }
}

void fill_fbm(struct unix_filesystem* ufs) {
  // For each inode
  for (int i = ufs->ibm->min; i < ufs->ibm->max; ++i) {
    // If allocated
    if (bm_get(ufs->ibm, i)) {
      // Set the sector that contains the inode
      bm_set(ufs->fbm, (uint64_t)i/INODES_PER_SECTOR + ufs->fbm->min);

      struct filev6 fv6;
  		memset(&fv6, 255, sizeof(fv6));

  		// Try to open the filev6 at the found inode
  		int openFile = filev6_open(ufs, i, &fv6);
      if (openFile < 0) continue;

      // Get the size
      int fileSize = inode_getsize(&(fv6.i_node));
      // if we have a medium file, we start by "allocating" every indirect sector in the fbm
      if (fileSize/SECTOR_SIZE > ADDR_SMALL_LENGTH &&  fileSize/SECTOR_SIZE < (ADDR_SMALL_LENGTH-1)*ADDRESSES_PER_SECTOR) {
        for (int j = 0; j < ADDR_SMALL_LENGTH; ++j) {
          
          bm_set(ufs->fbm, (fv6.i_node).i_address[j]);
        }
      }

      // Now we access to every data sector
      while(1) {
        
        // Find the next data sector
        int mySector = inode_findsector(fv6.u, &(fv6.i_node), fv6.offset / SECTOR_SIZE);
        // If error or offset out of the file, exit the loop
        if (mySector < 0 || fv6.offset >= fileSize) break;
        
        // "Allocate" the data sector in the fbm
        else bm_set(ufs->fbm, (uint64_t) mySector);
        
        // Move the offset to the new position
        int toMove = fv6.offset + SECTOR_SIZE > fileSize ? fileSize % SECTOR_SIZE : SECTOR_SIZE;
        fv6.offset += toMove;
      }
    }
  }
}
