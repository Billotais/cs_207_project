#include "filev6.h"
#include "unixv6fs.h"
#include "mount.h"
#include "inode.h"
#include "error.h"
#include "sector.h"

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
int filev6_lseek(struct filev6 *fv6, int32_t offset);

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
    int toMove = fv6->offset + SECTOR_SIZE >= fileSize ? fileSize % SECTOR_SIZE : SECTOR_SIZE;
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
int filev6_create(struct unix_filesystem *u, uint16_t mode, struct filev6 *fv6);

/**
 * @brief write the len bytes of the given buffer on disk to the given filev6
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on errror
 */
int filev6_writebytes(struct unix_filesystem *u, struct filev6 *fv6, void *buf, int len);
