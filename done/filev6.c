#include "filev6.h"
#include "unixv6fs.h"
#include "mount.h"
#include "inode.h"
#include "error.h"
#include "sector.h"
#include <string.h>


/**
 * @brief open up a file corresponding to a given inode; set offset to zero
 * @param u the filesystem (IN)
 * @param inr he inode number (IN)
 * @param fv6 the complete filve6 data structure (OUT)
 * @return 0 on success; <0 on errror
 */
int filev6_open(const struct unix_filesystem *u, uint16_t inr, struct filev6 *fv6) {
  // Check that arguments are not null
  M_REQUIRE_NON_NULL(u);
  M_REQUIRE_NON_NULL(fv6);
  if(inr < ROOT_INUMBER)return ERR_BAD_PARAMETER;

  // Set the filesystem in the filev6
  fv6->u = u;
  // Set the inode number to the given one
  fv6->i_number = inr;
  // Try to read inode #inr and save it in the inode from the filev6
  int resultOfRead = inode_read(u, inr, &(fv6->i_node));
  // if error return it
  if (resultOfRead != 0) return resultOfRead;

  // Initialize the offset
  fv6->offset = 0;

  return 0;
}

/**
 * @brief change the current offset of the given file to the one specified
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @param off the new offset of the file
 * @return 0 on success; <0 on errror
 */
int filev6_lseek(struct filev6 *fv6, int32_t offset) {
  // If the offset is bigger than the size of the file, error
	if (offset > inode_getsize(&(fv6->i_node))) return ERR_OFFSET_OUT_OF_RANGE;
  // else, just move the filev6 offset to the given position
	fv6->offset = offset;
	return 0;
}

/**
 * @brief read at most SECTOR_SIZE from the file at the current cursor
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @param buf points to SECTOR_SIZE bytes of available memory (OUT)
 * @return >0: the number of bytes of the file read; 0: end of file; <0 error
 */
int filev6_readblock(struct filev6 *fv6, void *buf) {
  // Check that arguments are not null
  M_REQUIRE_NON_NULL(fv6);
  M_REQUIRE_NON_NULL(buf);

  // Get filesize from the filev6
  int fileSize = inode_getsize(&(fv6->i_node));
  // If the offset is bigger that the size of the file, return 0 = end of file
  if (fv6->offset >= fileSize) return 0;

  // Get the sector number corresponding to the inode we try to read
  int mySector = inode_findsector(fv6->u, &(fv6->i_node), fv6->offset / SECTOR_SIZE);
  // If error while finding, return it
  if (mySector < 0) {
    return mySector;
  }
  // try to read the sector
  int readResult = sector_read((fv6->u)->f, mySector, buf);

  // If error return it
  if (readResult < 0) return readResult;


  // Move the offset to the new position
  int toMove = fv6->offset + SECTOR_SIZE > fileSize ? fileSize % SECTOR_SIZE : SECTOR_SIZE;
  fv6->offset += toMove;

  return toMove;
}

/**
 * @brief create a new filev6
 * @param u the filesystem (IN)
 * @param mode the new offset of the file
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @return 0 on success; <0 on errror
 */
int filev6_create(struct unix_filesystem *u, uint16_t mode, struct filev6 *fv6){
  M_REQUIRE_NON_NULL(u);
  M_REQUIRE_NON_NULL(fv6);
  
  //struct inode inode;
 // memset(fv6->i_node, 0, sizeof(struct inode));
  fv6->i_node.i_mode = mode;
  int write = inode_write(u, fv6->i_number, &(fv6->i_node));
  if (write != 0) return write;
  
  return 0;
}


// Helper function for filev6_writebytes
/**
 * @brief write one sector of data from buf in the filev6
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on errror
 */

int filev6_writesector(struct unix_filesystem *u, struct filev6 *fv6, void *buf, int len) {
  M_REQUIRE_NON_NULL(u);
  M_REQUIRE_NON_NULL(fv6);
  M_REQUIRE_NON_NULL(buf);
  if(len < 0) return ERR_BAD_PARAMETER;

  size_t nb_bytes;
  // Current size of the filev6
  size_t fileSize = inode_getsize(&fv6->i_node);
  fv6->offset = fileSize;
  
  // If size bigger than a small file not handled
  if (fileSize + len > (ADDR_SMALL_LENGTH-1)*SECTOR_SIZE*ADDRESSES_PER_SECTOR) return ERR_FILE_TOO_LARGE;
   
  // If the last sector was full
  if (fileSize % SECTOR_SIZE == 0) {
      
      // We'll write at most SECTOR_SIZE bytes
      nb_bytes = len >= SECTOR_SIZE ? SECTOR_SIZE : len;
      
      // Find a free sector
      int freeSector = bm_find_next(u->fbm);

      if (freeSector < 0) return ERR_BITMAP_FULL;

      // Tell fbm that we'll use it 
      bm_set(u->fbm, freeSector);
      
      // Write opur data in this sector, if we can't free in the fbm
      int writeSector = sector_write(u->f, freeSector, buf);
      if (writeSector < 0) {bm_clear(u->fbm, freeSector);return writeSector;}

      // Update the address array
      fv6->i_node.i_address[fileSize / SECTOR_SIZE] = freeSector; 
      // Move the offset
      fv6->offset += nb_bytes;    
  }
  else { // If the last sector is not full yet
    
    // We write until the sector is full
    nb_bytes = (SECTOR_SIZE - fileSize % SECTOR_SIZE >= len) ? len : (SECTOR_SIZE - fileSize % SECTOR_SIZE);
    
    // Get the sector address
    int sectorAddress = fv6->i_node.i_address[fileSize / SECTOR_SIZE];
  
    // read the sector
    uint8_t data[SECTOR_SIZE];
    int readData = sector_read(u->f, sectorAddress, data);
    if (readData < 0) return readData;

    // Cast to byte to manipulate data
    uint8_t* dataBytes = (uint8_t*) data;
    uint8_t* bufBytes = (uint8_t*) buf; 


    // Copy every byte
    for (size_t i = 0; i < nb_bytes; ++i) {
      dataBytes[fileSize % SECTOR_SIZE + i] = bufBytes[i];
    }
  
    // rewrite the sector
    int tryWrite = sector_write(u->f, sectorAddress, data);
    if (tryWrite < 0) return tryWrite;

    fv6->offset += nb_bytes;
    

  }
  // Update the size of the inode
  int changeSize = inode_setsize(&(fv6->i_node), fileSize + nb_bytes);
  if (changeSize < 0) return changeSize;
 
  return nb_bytes;
  
}

/**
 * @brief write the len bytes of the given buffer on disk to the given filev6
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on errror
 */
int filev6_writebytes(struct unix_filesystem *u, struct filev6 *fv6, void *buf, int len) {
  M_REQUIRE_NON_NULL(u);
  M_REQUIRE_NON_NULL(fv6);
  M_REQUIRE_NON_NULL(buf);
  if(len < 0) return ERR_BAD_PARAMETER;
  
  size_t leftLen = len;
  // While we still have some bytes to write
  while (leftLen != 0) {
    // Try to write
    int writen = filev6_writesector(u, fv6, ((uint8_t*) buf) + len - leftLen, leftLen) ;
    if (writen < 0) return writen;
    // If success, reduce leftLen
    leftLen -= writen;

  }
  
  // Finaly write the inode 
  int writeInode = inode_write(u, fv6->i_number, &(fv6->i_node));
  if (writeInode < 0) return writeInode;
 
  return 0;
}
