#include <openssl/sha.h>
#include "error.h"
#include <string.h>
#include "filev6.h"
#include "unixv6fs.h"
#include "mount.h"
#include "inode.h"
#include "error.h"
#include "sector.h"



/**
 * @brief transform SHA into string
 * @param SHA the SHA we want to print
 * @param sha_string the destination of the string computed from the SHA
 */

 static void sha_to_string(const unsigned char *SHA, char *sha_string)
 {
     if ((SHA == NULL) || (sha_string == NULL)) {
         return;
     }

     for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
         sprintf(&sha_string[i * 2], "%02x", SHA[i]);
     }

     sha_string[2 * SHA256_DIGEST_LENGTH] = '\0';
 }

/**
 * @brief print the sha of the content
 * @param content the content of which we want to print the sha
 * @param length the length of the content
 */
void print_sha_from_content(const unsigned char *content, size_t length) {

  unsigned char result[SHA256_DIGEST_LENGTH] = "";
  SHA256(content, length, result);
  char string[SHA256_DIGEST_LENGTH*2 + 1] = "";
  sha_to_string(result, string);
  printf("%s\n", string);
}



/**
 * @brief print the sha of the content of an inode
 * @param u the filesystem
 * @param inode the inode of which we want to print the content
 * @param inr the inode number
 */
void print_sha_inode(struct unix_filesystem *u, struct inode inode, int inr) {

  if (inode.i_mode & IALLOC) {
    printf("SHA inode %d: ", inr);
    if (inode.i_mode & IFDIR) {
      printf("no SHA for directories.\n");
    }
    else {
      struct filev6 stv6;
      memset(&stv6, 255, sizeof(stv6));

      int inodeOpen = filev6_open(u, inr, &stv6);
      if(inodeOpen >= 0) {

        int inode_size = inode_getsize(&inode);
        //TODO
        unsigned char data[((inode_size / SECTOR_SIZE) + 1)*SECTOR_SIZE + 1];

        unsigned char *ptr = data;

        int fileRead;

        do {
          fileRead = filev6_readblock(&stv6, ptr);
          if (fileRead < 0) {
            fprintf(stderr, "Error while reading block for inode");
          }
          ptr += SECTOR_SIZE;
        } while (fileRead > 0);
        data[inode_size] = '\0';


        print_sha_from_content(data, inode_size);
        ptr = NULL;

      }
    }
  }

}
