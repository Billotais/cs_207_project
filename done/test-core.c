/**
 * @file test-core.c
 * @brief main program to perform tests on disks (mainly for weeks 04 to 06)
 *
 * @author Aurélien Soccard & Jean-Cédric Chappelier
 * @date 15 Oct 2016
 */

#include <stdlib.h>
#include <stdio.h>
#include "mount.h"
#include "error.h"

#define MIN_ARGS 1
#define MAX_ARGS 1
#define USAGE    "test <diskname>"

int test(struct unix_filesystem *u);

void error(const char* message)
{
    fputs(message, stderr);
    putc('\n', stderr);
    fputs("Usage: " USAGE, stderr);
    putc('\n', stderr);
    exit(1);
}

void check_args(int argc)
{
    if (argc < MIN_ARGS) {
        error("too few arguments:");
    }
    if (argc > MAX_ARGS) {
        error("too many arguments:");
    }
}

int main(int argc, char *argv[])
{
    // Check the number of args but remove program's name
    check_args(argc - 1);

    FILE* f = fopen(argv[1], "rb");
    fseek(f, 1408, SEEK_SET);
    uint16_t mode = 0;
    fread(&mode, 2, 1, f);
    printf("%d\n", (int) mode);
    fclose(f);

    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u);
    if (error == 0) {
        mountv6_print_superblock(&u);
        error = test(&u);
    }
    if (error) {
        puts(ERR_MESSAGES[error - ERR_FIRST]);
    }
     fseek(u.f, 1408, SEEK_SET);
    uint16_t mode2 = 0;
    fread(&mode2, 2, 1, u.f);
    printf("After mount%d\n", (int) mode2);
    umountv6(&u); /* shall umount even if mount failed,
                   * for instance fopen could have succeeded
                   * in mount (thus fclose required).
                   */
    FILE* f1 = fopen(argv[1], "rb");
    fseek(f1, 1408, SEEK_SET);
    uint16_t mode1 = 0;
    fread(&mode1, 2, 1, f1);
    printf("%d\n", (int) mode1);   
    fclose(f1);                
    return 0;
}
