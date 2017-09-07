/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26
#define MAXPATHLEN_UV6 1024

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include "mount.h"
#include "unixv6fs.h"
#include "inode.h"
#include "error.h"
#include "direntv6.h"
#include "filev6.h"
struct unix_filesystem fs;

static int fs_getattr(const char *path, struct stat *stbuf)
{
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(stbuf);

    // Find the inode correspondig to the given path
    int inode = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
    // Return any error
    if (inode < 0) return inode;

	  // Try to read the inode
    struct inode readInode;
    int tryRead = inode_read(&fs, inode, &readInode);
    if (tryRead < 0) return tryRead;

	  
    memset(stbuf, 0, sizeof(struct stat));

	  // Set all fields used by FUSE
    stbuf->st_dev = 0;
    stbuf->st_ino = inode;
    stbuf->st_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH | ((readInode.i_mode & IFDIR) ? S_IFDIR : S_IFREG);
    stbuf->st_nlink = readInode.i_nlink;
    stbuf->st_uid = readInode.i_uid;
    stbuf->st_gid = readInode.i_gid;
    stbuf->st_rdev = 0;
    stbuf->st_size = inode_getsize(&readInode);
    stbuf->st_blksize = SECTOR_SIZE;
    stbuf->st_blocks = (stbuf->st_size - 1)/ stbuf->st_blksize + 1;

    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi)
{
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(buf);
    M_REQUIRE_NON_NULL(fi);

    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    // Find the inode correspondig to the given path
    int inode = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
    // Return any error
    if (inode < 0) return inode;
    // Create a new directpry_reader
    struct directory_reader dir;

    // Try to open directory correponding to inode inr
    int tryOpen = direntv6_opendir(&fs, inode, &dir);

    // If other type of error, return it
    if(tryOpen < 0) return tryOpen;

    int tryRead;

    // Do all this while we can read childs from the current directory
    //correcteur: pas de check ALLOC ou IFDIR : -1

    do {
        // Initialize name to know name of next child
        char name[DIRENT_MAXLEN+1];
        // Not useful but needed by readdir
        uint16_t nextChild;


        // Try to read a child of the current dir
        tryRead = direntv6_readdir(&dir, name, &nextChild);
        // If we can't, return error code (either error or no more child)
        if(tryRead <= 0) return tryRead;

        // Write the name
        char toPrint[MAXPATHLEN_UV6+1];
        snprintf(toPrint, MAXPATHLEN_UV6, "%s", name);
        toPrint[MAXPATHLEN_UV6] = '\0';

		    // Send the name to FUSE
        filler(buf, toPrint, NULL, 0);

    } while(tryRead == 1);

    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi)
{
    //correcteur: SEGFAULT : -1
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(buf);
    M_REQUIRE_NON_NULL(fi);
    (void) fi;

	  // Find the corresponding inode
    int inode = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
    // Return any error
    if (inode < 0) return inode;

    // Create a new filev6, and set it's memory to a default value
    struct filev6 stv6;
    memset(&stv6, 255, sizeof(stv6));

    // Try to open the filev6 at the found inode
    int inodeOpen = filev6_open(&fs, inode, &stv6);
    if(inodeOpen < 0) return inodeOpen;

	  // Move the cursor at the corret position gieven by FUSE
    int fileSeek = filev6_lseek(&stv6, offset);
    if (fileSeek < 0) return fileSeek;

    unsigned char data[SECTOR_SIZE + 1];
	data[SECTOR_SIZE] = '\0';

////correcteur: Doit lire le plus possible : -1
//correcteur: pas de check de ALLOC ou IFDIR : -1
    int fileRead;

	  // Read the next block of data pointed by the cursor
    fileRead = filev6_readblock(&stv6, &data);
    // Set last char to \0 to end the string
    data[size - 1] = '\0';
    // If error return it
    if(fileRead < 0) return fileRead;
    // If fileRead > 0 => we did read successfully => save what we read
    if(fileRead > 0) memcpy(buf, data, fileRead);

    // return how much bytes we read
    return fileRead;
}

static struct fuse_operations available_ops = {
    .getattr	= fs_getattr,
    .readdir	= fs_readdir,
    .read		= fs_read,
};


/* From https://github.com/libfuse/libfuse/wiki/Option-Parsing.
 * This will look up into the args to search for the name of the FS.
 */
static int arg_parse(void *data, const char *filename, int key, struct fuse_args *outargs)
{
    (void) data;
    (void) outargs;
    if (key == FUSE_OPT_KEY_NONOPT && fs.f == NULL && filename != NULL) {


        int tryMount = mountv6(filename, &fs);
        // If we can't error
        if (tryMount < 0) {
            // print error and exit fuse
            fprintf(stderr,"Error : %s\n", ERR_MESSAGES[tryMount - ERR_FIRST]);
            exit(1);
        }
        return 0;
    }
    return 1;
}
int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int ret = fuse_opt_parse(&args, NULL, NULL, arg_parse);
    if (ret == 0) {
        ret = fuse_main(args.argc, args.argv, &available_ops, NULL);
        (void)umountv6(&fs);
    }
    return ret;
}
