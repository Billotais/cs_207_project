#include "unixv6fs.h"
#include "mount.h"
#include "error.h"
#include "sector.h"
#include "inode.h"
#include <inttypes.h>

/**
 * @brief set the size of a given inode to the given size
 * @param inode the inode
 * @param new_size the new size
 * @return 0 on success; <0 on error
 */
int inode_setsize(struct inode *inode, int new_size);

/**
 * @brief prints the content of an inode structure
 * @param inode the inode structure to be displayed
 */
void inode_print(const struct inode *inode) {
    printf("*********FS INODE START**********\n");
    // If pointer to inode is null
    if (inode == NULL) {
        printf("NULL ptr\n");
    }
    else {
        printf("i_mode: %"PRIu16"\n", inode->i_mode);
        printf("i_nlink: %"PRIu8"\n", inode->i_nlink);
        printf("i_uid: %"PRIu8"\n", inode->i_uid);
        printf("i_gid: %"PRIu8"\n", inode->i_gid);
        printf("i_size0: %"PRIu8"\n", inode->i_size0);
        printf("i_size1: %"PRIu16"\n", inode->i_size1);
        printf("size: %"PRIu32"\n", inode->i_size0 + inode->i_size1);
    }
    printf("**********FS INDOE END**********\n");
}

/**
 * @brief read all inodes from disk and print out their content to
 *        stdout according to the assignment
 * @param u the filesystem
 * @return 0 on success; < 0 on error.
 */
int inode_scan_print(const struct unix_filesystem *u) {
    M_REQUIRE_NON_NULL(u);

    // Go through every sector of the filessystem
    for (int i = (u->s).s_inode_start; i < (u->s).s_inode_start + (u->s).s_isize; ++i) {

        //correcteur: -1 : pourquoi ne pas juste utiliser un tableau de struct 
        uint16_t toBeRead[256];

        // Read the ith sector
        int sector = sector_read(u->f, i, toBeRead);
        if (sector != 0) {
            return sector;
        }

        // Go through each inode of the current sector
        // Since the toBeaRead array is composed of 16 buts elements, only need to go
        // for SECTOR_SIZE / 2 elements
        for (int j = 0; j < SECTOR_SIZE / 2; j += (SECTOR_SIZE / 2)/INODES_PER_SECTOR) {

            // For each one, create a temporary inode and initiliazie some fields
            struct inode readInode;
            readInode.i_mode = toBeRead[j];
            readInode.i_size0 = toBeRead[j+2] >> 8;
            readInode.i_size1 = toBeRead[j+3];


            // If the inode exists
            if (readInode.i_mode & IALLOC) {
                // Print
                //correcteur: [M] valeurs hardcodée
                printf("inode   %d (%s) len   %d\n",
                        (i-(u->s).s_inode_start)*16 + j/16, //inode number
                        (readInode.i_mode & IFDIR) ? SHORT_DIR_NAME : SHORT_FIL_NAME, // type of file of the inode
                        inode_getsize(&readInode)); // size of the inode
            }
        }
    }
    return 0;
}

/**
 * @brief read the content of an inode from disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (OUT)
 * @return 0 on success; <0 on error
 */
int inode_read(const struct unix_filesystem *u, uint16_t inr, struct inode *inode) {
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(inode);
    ////correcteur: [M] utiliser un tableau de struct !!
    //correcteur: pas de check inr < ROOT_INUMBER : -1

    // If the given inode number is >= than the number of allocated inodes, it is unallocated
    if ((u->s).s_isize * INODES_PER_SECTOR <= inr) return ERR_UNALLOCATED_INODE;

    // Get the sector where the inode is
    int correctSector = inr / INODES_PER_SECTOR + (u->s).s_inode_start;
    uint16_t toBeRead[256];

    // Read the correct sector
    int sector = sector_read(u->f, correctSector, toBeRead);
    if (sector != 0) return sector;

    // Get the inode position inside the sector
    int posOfInode = (inr % INODES_PER_SECTOR)*INODES_PER_SECTOR;
    // Check if the inode is allocated (first check should be ok, but it is great to double check)
    if (!(toBeRead[posOfInode] & IALLOC)) return ERR_UNALLOCATED_INODE;

    // initialize all fields of the inode
    //correcteur: [M] Non, c'est sale
    inode->i_mode = toBeRead[posOfInode];
    inode->i_nlink = toBeRead[posOfInode + 1] & 0xff;
    inode->i_uid = toBeRead[posOfInode + 1] >> 8;
    inode->i_gid = toBeRead[posOfInode + 2] & 0xff;
    inode->i_size0 = toBeRead[posOfInode + 2] >> 8;
    inode->i_size1 = toBeRead[posOfInode + 3];

    for (int i = 0; i < ADDR_SMALL_LENGTH; ++i) {
        inode->i_address[i] = toBeRead[posOfInode + 4 + i];
    }

    inode->i_atime[0] = toBeRead[posOfInode + ADDR_SMALL_LENGTH + 4];
    inode->i_atime[1] = toBeRead[posOfInode + ADDR_SMALL_LENGTH + 5];
    inode->i_mtime[0] = toBeRead[posOfInode + ADDR_SMALL_LENGTH + 6];
    inode->i_mtime[1] = toBeRead[posOfInode + ADDR_SMALL_LENGTH + 7];

    return 0;
}

/**
 * @brief identify the sector that corresponds to a given portion of a file
 * @param u the filesystem (IN)
 * @param inode the inode (IN)
 * @param file_sec_off the offset within the file (in sector-size units)
 * @return >0: the sector on disk;  <0 error
 */
int inode_findsector(const struct unix_filesystem *u, const struct inode *i, int32_t file_sec_off) {
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(i);
    // Check if the inode is allocated
    if (! (i->i_mode & IALLOC)) return ERR_UNALLOCATED_INODE;
    // If number of allocated sectors > 7*256 sectors => file too big
    //correcteur: [M] valeurs hardcodée 
    if (inode_getsize(i) / SECTOR_SIZE > 7*ADDRESSES_PER_SECTOR) return ERR_FILE_TOO_LARGE;
    // If the offset is not valid
    if (file_sec_off / SECTOR_SIZE >= inode_getsize(i)) return ERR_OFFSET_OUT_OF_RANGE;

    // If we have a small file
    if (inode_getsize(i) / SECTOR_SIZE <= 8) {
        // Directly read address from i_address at position file_sec_off
        //correcteur: pas de check de file_sec_off : -2
        return i->i_address[file_sec_off];
    }
    // if (inode_getsize(i) >= 9 && inode_getsize(i) <= 7*ADDRESSES_PER_SECTOR) => big file
    else {
        // Get the indirect sector
        int sectorAddress = i->i_address[file_sec_off / ADDRESSES_PER_SECTOR];

        uint16_t toBeRead[INODES_PER_SECTOR*sizeof(struct inode)/sizeof(uint16_t)];

        // Read the correct sector
        int sector = sector_read(u->f, sectorAddress, toBeRead);
        if (sector != 0) return sector;

        return toBeRead[file_sec_off % ADDRESSES_PER_SECTOR];
    }
}

/**
 * @brief alloc a new inode (returns its inr if possible)
 * @param u the filesystem (IN)
 * @return the inode number of the new inode or error code on error
 */
int inode_alloc(struct unix_filesystem *u);

/**
 * @brief write the content of an inode to disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (IN)
 * @return 0 on success; <0 on error
 */
int inode_write(struct unix_filesystem *u, uint16_t inr, struct inode *inode);
