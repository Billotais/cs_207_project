CFLAGS+= -std=c99 -Wall -pedantic  -g
LDLIBS += -lcrypto
# correcteur : évitez les .c dans le makefile : -1
all: test-inodes test-inode-read test-file test-dirent shell clean
shell: shell.o inode.o sector.o direntv6.o mount.o filev6.o error.o sha.o
test-inodes: test-core.o error.o test-inodes.o mount.o inode.o sector.o filev6.o
test-inode-read: test-core.o error.o test-inode-read.o mount.o inode.o sector.o filev6.o
test-file: test-file.o test-core.o error.o mount.o inode.o filev6.o sha.o sector.o 
test-dirent: test-dirent.o test-core.o error.o mount.o inode.o filev6.o direntv6.o sector.o
shell.o: shell.c mount.h unixv6fs.h bmblock.h direntv6.h filev6.h inode.h error.h sha.h
direntv6.o: direntv6.c unixv6fs.h filev6.h mount.h bmblock.h error.h \
 direntv6.h
error.o: error.c
filev6.o: filev6.c filev6.h unixv6fs.h mount.h bmblock.h inode.h error.h \
 sector.h
inode.o: inode.c unixv6fs.h mount.h bmblock.h error.h sector.h inode.h
mount.o: mount.c unixv6fs.h bmblock.h mount.h error.h sector.h
sector.o: sector.c unixv6fs.h error.h
sha.o: sha.c error.h filev6.h unixv6fs.h mount.h bmblock.h inode.h \
 sector.h
test-core.o: test-core.c mount.h unixv6fs.h bmblock.h error.h
test-dirent.o: test-dirent.c direntv6.h unixv6fs.h filev6.h mount.h \
 bmblock.h error.h inode.h
test-file.o: test-file.c filev6.h unixv6fs.h mount.h bmblock.h inode.h \
 sha.h
test-inode-read.o: test-inode-read.c inode.h unixv6fs.h mount.h bmblock.h
test-inodes.o: test-inodes.c inode.h unixv6fs.h mount.h bmblock.h
clean:
	rm -f *.o
