#include <stdint.h>
#include "unixv6fs.h"
#include "filev6.h"
#include "mount.h"
#include "error.h"
#include "string.h"
#include "direntv6.h"
#include <inttypes.h>

#define MAXPATHLEN_UV6 1024
/**
 * @brief opens a directory reader for the specified inode 'inr'
 * @param u the mounted filesystem
 * @param inr the inode -- which must point to an allocated directory
 * @param d the directory reader (OUT)
 * @return 0 on success; <0 on errror
 */
int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d){

  M_REQUIRE_NON_NULL(u);
  M_REQUIRE_NON_NULL(d);

  // Try to open the file
  int openFile = filev6_open(u, inr, &(d->fv6));
  // Return error if we can't open it
  if (openFile < 0) {return openFile;}
  
  // If the inode doesn't correpond to a dirctory, error
  //correcteur: ne fonctionne pas : -0.5 
  if (!((d->fv6.i_node).i_mode & IFDIR)) {return ERR_INVALID_DIRECTORY_INODE;}
  // Initialize cur and last at the beggining
  d->cur = 0;
  d->last = 0;

  return 0;
}

/**
 * @brief return the next directory entry.
 * @param d the dierctory reader
 * @param name pointer to at least DIRENTMAX_LEN+1 bytes.  Filled in with the NULL-terminated string of the entry (OUT)
 * @param child_inr pointer to the inode number in the entry (OUT)
 * @return 1 on success;  0 if there are no more entries to read; <0 on error
 */
int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr) {
  M_REQUIRE_NON_NULL(d);
  M_REQUIRE_NON_NULL(name);
  M_REQUIRE_NON_NULL(child_inr);
  
  int blockRead;
  // If we've reached the end of a sector
  if (d->cur == d->last) {
	// Create a array of direntv6 to store all dirs/files of the next sector
    struct direntv6 data[SECTOR_SIZE / sizeof(struct direntv6)];
    // Try to read the next sector
    blockRead = filev6_readblock(&(d->fv6), data);
    // If error or end of file, return error or 0
    if (blockRead <= 0) {return blockRead;}
	// Go through each direntv6 in our array and copy their values to d->dirs
    for (int i = 0; i < blockRead/sizeof(struct direntv6); ++i) {
	  // Copy d_inumber
      d->dirs[i].d_inumber = data[i].d_inumber;
	  // Copy d_name
      strncpy(d->dirs[i].d_name, data[i].d_name, DIRENT_MAXLEN);
    }
	// Move the last cursor to the end of the read sector
    d->last += blockRead/sizeof(struct direntv6);

  }
  // If we're not at the end of a sector
  
  // Copy the d_name from d->dirs to "output" name
  strncpy(name,(d->dirs[d->cur % (SECTOR_SIZE/sizeof(struct direntv6))]).d_name, DIRENT_MAXLEN);
  name[DIRENT_MAXLEN] = '\0';
  // Copy the d_inumber into child_inr
  *child_inr = ((d->dirs[d->cur % (SECTOR_SIZE/sizeof(struct direntv6))]).d_inumber);

  // Move the cursor to ther next dir
  d->cur += 1 ;

  return 1;
}

/**
 * @brief debugging routine; print the a subtree (note: recursive)
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param prefix the prefix to the subtree
 * @return 0 on success; <0 on error
 */
int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix) {
  M_REQUIRE_NON_NULL(u);
  M_REQUIRE_NON_NULL(prefix);
	
  // Create a new directpry_reader	
  struct directory_reader dir;

  // Try to open directory correponding to inode inr
  int tryOpen = direntv6_opendir(u, inr, &dir);

  // If we can't open the directoy and that the error is INVALID_DIRECTORY_INODE
  // It means it is a file => print FIL "prefix"
  if(tryOpen == ERR_INVALID_DIRECTORY_INODE) {
	  printf("FIL %s\n", prefix);
	  return 0;
  }
  // If other type of error, return it
  else if(tryOpen < 0) {return tryOpen;}
  
  // If we manage to openn the directory, print its "address"
  printf("DIR %s/\n", prefix);

  int tryRead;
  
  // Do all this while we can read childs from the current directory
  
  do {
	// Initialize name and nextChild to give theme to readdir  
    char name[DIRENT_MAXLEN+1];
    uint16_t nextChild;

	// Try to read a child of the current dir
    tryRead = direntv6_readdir(&dir, name, &nextChild);
	// If we can't, return error code (either error or no more child)
    if(tryRead <= 0) return tryRead;
    
    // Write into toPrint the next prefix name = prefix/name
    char toPrint[MAXPATHLEN_UV6+1];
    snprintf(toPrint, MAXPATHLEN_UV6, "%s/%s", prefix, name);
    toPrint[MAXPATHLEN_UV6] = '\0';


    // Try to recurse on the current child
    int tryRecurs = direntv6_print_tree(u, nextChild, toPrint);
    // Return errors from print_tree
    if(tryRecurs != 0) return tryRecurs;

  } while(tryRead == 1);

  return 0;
}

/**
 * @brief get the inode number for the given path
 * @param u a mounted filesystem
 * @param inr the current of the subtree
 * @param entry the prefix to the subtree
 * @return inr on success; <0 on error
 */
 int direntv6_dirlookup_core(const struct unix_filesystem *u, uint16_t inr, const char *entry, size_t size){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(entry);
	if (strlen(entry) == 0) return inr;
	if (entry[0] == '/') {
		int i = 1;
		while(i < size && entry[i] == '/') {
			++i;
		}
		return direntv6_dirlookup_core(u, inr, entry + i, size - i);
	}
	else {

		int sizeToCompare;
		char *endDirName = strchr(entry, '/');
		if (endDirName == NULL) sizeToCompare = size;
		else sizeToCompare = endDirName - entry;


		struct directory_reader dir;
		int tryOpenDir = direntv6_opendir(u, inr, &dir);
		if (tryOpenDir < 0) return tryOpenDir;

		int tryRead;
		uint16_t nextChild;
		char name[DIRENT_MAXLEN+1];
		int found = 0;
		do{
			tryRead = direntv6_readdir(&dir, name, &nextChild);
			if(tryRead < 0){return tryRead;}
			if (strncmp(name, entry, sizeToCompare) == 0) found = 1;

		} while(tryRead == 1 && found == 0);

		if (found == 0) return ERR_INODE_OUTOF_RANGE;

		if (endDirName == NULL) {
			return nextChild;
		}
		else {
			return direntv6_dirlookup_core(u, nextChild, endDirName, size - sizeToCompare);
		}
	}
}

int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(entry);
	return direntv6_dirlookup_core(u, inr, entry, strlen(entry));
}

/**
 * @brief create a new direntv6 with the given name and given mode
 * @param u a mounted filesystem
 * @param entry the path of the new entry
 * @param mode the mode of the new inode
 * @return inr on success; <0 on error
 */
int direntv6_create(struct unix_filesystem *u, const char *entry, uint16_t mode);
