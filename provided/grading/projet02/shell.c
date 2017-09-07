
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "mount.h"
#include "unixv6fs.h"
#include "direntv6.h"
#include "inode.h"
#include "error.h"
#include "sha.h"
#define CMD_NB 13

//MAX_ARGS = 5 : name_of_function + max_3_args (in the function with the most args) + 1 (to check if there isn't any 5th or more arg)
#define MAX_ARGS 5
#define ERR_EXIT_CODE 100
#define ERR_INR_OUT_OF_RANGE 101
#define ERR_NOT_MOUNTED 102
#define ERR_CAT_DIR 103
#define ERR_NON_VALID_ARG 104


typedef int (*shell_fct)(const char**);

struct shell_map {
	const char* name;
	shell_fct  fct;
	const char* help;
	size_t argc;
	const char* args;
};

int do_help(const char** c);
int do_exit(const char** c);
int do_mkfs(const char** c);
int do_lsall(const char** c);
int do_add(const char** c);
int do_mkdir(const char** c);
int do_mount(const char** c);
int do_cat(const char** c);
int do_istat(const char** c);
int do_inode(const char** c);
int do_sha(const char** c);
int do_psb(const char** c);

struct unix_filesystem u = {0};
int FS_mounted = 0;

struct shell_map shell_cmds[CMD_NB] = {
	{"help", do_help, "display this help", 0, ""},
	{"exit", do_exit, "exit shell", 0, ""},
	{"quit", do_exit, "exit shell", 0, ""},
	{"mkfs", do_mkfs, "create a new filesystem", 3, "<diskname> <#inode> <#blocks>"},
	{"lsall", do_lsall, "list all directories and files containes in the currently mounted filesystem", 0, ""},
	{"add", do_add, "add a new file", 2, "<src-fullpath> <dst>"},
	{"mkdir", do_mkdir, "create a new directory", 1, "<dirname>"},
	{"mount", do_mount, "mount the provided filesystem", 1, "<diskname>"},
	{"cat", do_cat, "display the content of a file", 1, "<pathname>"},
	{"istat", do_istat, "display information about the provided inode", 1, "<inode_nr>"},
	{"inode", do_inode, "display the inode number of a file", 1, "<pathname>"},
	{"sha", do_sha, "display the SHA of a file", 1, "<pathname>"},
	{"psb", do_psb, "Print superBlock of the currently mounted filesystem", 0, ""}
};

// Separate all arguments of our command
// Return the number of elements tokenized

int tokenize_input(char* input, const char** output) {
	char* tryToTokensize = input;
	size_t i = 0;
	while(tryToTokensize != NULL && i < MAX_ARGS) {
		if(i == 0) tryToTokensize = strtok(input, " ");
		else tryToTokensize = strtok(NULL, " ");
		if(tryToTokensize != NULL) output[i++] = tryToTokensize;
	}
	return i;

}
int main(void) {
	// Boolean used to end the loop when exit/quit
	int running = 1;

	// While no error on input and still running
	while(!feof(stdin) && !ferror(stdin) && running) {

		// Read what is written and replace the \n by a \0
		char read[256];
		printf("> ");
		fgets(read, 255, stdin);

		size_t ln = strlen(read) - 1;
		if (ln < 0) continue;
		if (read[ln] == '\n') read[ln] = '\0';

		// Create an array for tokenize (We don't have more than MAX_ARGS command/arguments)
		const char* args[MAX_ARGS];

		// Get the number of arguments
		int args_n = tokenize_input(read, args) - 1;
		int found = 0;
		int i = 0;

		if (found == -1) printf("ERROR SHELL: invalid command\n");
		else{
			// Go through all comands
			while (i < CMD_NB && found == 0) {
				// If we found a command with the correct name
				if (strcmp(args[0], shell_cmds[i].name) == 0) {
					// Update found so we know we don't need to look through other commands
					found = 1;
					// If we don't have the correct number of argumenets, error SHELL
					if(shell_cmds[i].argc != args_n) printf("ERROR SHELL: wrong number of arguments\n");

					else {
						// Create an pointer to the corresponding function
						shell_fct my_fct = shell_cmds[i].fct;
						// Call the function with all the args (we do +1 because the first arg is the comand name)
						int tryFct = my_fct(args + 1);

						// Depending of the return value, exit, print SHELL error or print FS error
						if (tryFct == ERR_EXIT_CODE) running = 0;
						else if (tryFct == ERR_INR_OUT_OF_RANGE) printf("ERROR FS: inode out of range\n");
						else if (tryFct == ERR_NOT_MOUNTED) printf("ERROR SHELL: mount the FS before the operation\n");
						else if (tryFct == ERR_CAT_DIR) printf("ERROR SHELL: cat on a directory is not defined\n");
						else if (tryFct == ERR_IO) printf("ERROR FS: IO error: No such file or directory\n");
						else if (tryFct == ERR_NON_VALID_ARG) printf("ERROR FS: The given argument(s) is/are not valid\n");
						else if (tryFct < 0) printf("ERROR FS: %s\n", ERR_MESSAGES[tryFct - ERR_FIRST]);
					}
				}
				++i;
			}
		}
		// If we didn't found any correponding command, error shell
		if (found == 0) printf("ERROR SHELL: invalid command\n");
	}

	return 0;
}
int do_help(const char** c) {
	// Go through all commands and print a description
	for (int i = 0; i < CMD_NB; ++i) {
		if (shell_cmds[i].argc == 0) printf("- %s: %s.\n",shell_cmds[i].name, shell_cmds[i].help);
		else printf("- %s %s: %s.\n",shell_cmds[i].name,shell_cmds[i].args, shell_cmds[i].help);
	}
	return 0;
}
int do_mount(const char** c) {
	if (FS_mounted) umountv6(&u);

	FS_mounted = 0;
	// Try to mount with first arg == address of .uv6
    int tryMount = mountv6(c[0], &u);
    // If we can't error
    if (tryMount < 0)return tryMount;
    // New all other functions know that the filesystem if ready to be used
    FS_mounted = 1;

    return 0;
}

int do_lsall(const char** c) {
	// Check that filesystem is mounted
	if (!FS_mounted) {
		return ERR_NOT_MOUNTED;
	}
	// Simply print the tree of the root
	return direntv6_print_tree(&u, ROOT_INUMBER, "");
}
int do_psb(const char** c) {
	// Check that filesystem is mounted
	if (!FS_mounted) {
		return ERR_NOT_MOUNTED;
	}
	// Print superblock of unix_filesystem
	mountv6_print_superblock(&u);
	return 0;
}
int do_istat(const char** c){
	// Check that filesystem is mounted
	if (!FS_mounted) {
		return ERR_NOT_MOUNTED;
	}
	int inr;

	// Parse the string argument into an int
	int tryRead = sscanf(c[0], "%d", &inr);
	if (tryRead == EOF) return ERR_NON_VALID_ARG;

	// if inr < 1, we know that the inode is not valid (0 is not used)
	if (inr < 1) return ERR_INR_OUT_OF_RANGE;

	// Create empty inode and fill it by calling inode_read
	struct inode readInode;
	tryRead = inode_read(&u, inr, &readInode);
	if (tryRead < 0) return tryRead;

	// Then print then inode
	inode_print(&readInode);
	return 0;
}

int do_exit(const char** c){
	if (FS_mounted) umountv6(&u);
	return ERR_EXIT_CODE;
}
int do_mkfs(const char** c){return 0;}
int do_add(const char** c){return 0;}
int do_mkdir(const char** c){return 0;}
int do_cat(const char** c){
	// If the filesystem is not mounted yet, error
	if (!FS_mounted) {
		return ERR_NOT_MOUNTED;
	}
	// Find the inode correspondig to the given path
	int inode = direntv6_dirlookup(&u, ROOT_INUMBER, c[0]);
	// Return any error
	if (inode < 0) return inode;


	// Create a new filev6, and set it's memory to a default value
	struct filev6 stv6;
	memset(&stv6, 255, sizeof(stv6));

	// Try to open the filev6 at the found inode
	int inodeOpen = filev6_open(&u, inode, &stv6);
	if(inodeOpen < 0) return inodeOpen;
	else {
		// If the inode is a directory, error
		if (((stv6.i_node).i_mode & IFDIR)) return ERR_CAT_DIR;
		else {
			// Prepare an array to save data
			unsigned char data[SECTOR_SIZE + 1];

			int fileRead = 1;
			// While there is something to read
			while(fileRead > 0){
				// Try too read it
				fileRead = filev6_readblock(&stv6, &data);
				// Set last char to \0 to end the string
				data[SECTOR_SIZE] = '\0';
				// If eror return it
				if(fileRead < 0) return fileRead;
				// If fileRead > 0 => we did read successfully => print what we read
				if(fileRead > 0) printf("%s", data);
				// If fileRead == 0, we're at the end of the file => the loop will end
			}
		}
    }
    return 0;
}
int do_inode(const char** c){
	// Check that filesystem is mounted
	if (!FS_mounted) {
		return ERR_NOT_MOUNTED;
	}
	// Find inode correponsponding to path
	int inode = direntv6_dirlookup(&u, ROOT_INUMBER, c[0]);
	if (inode < 0) return inode;

	// Print it if we don't have any error
	printf("inode: %d\n", inode);
	return 0;
}
int do_sha(const char** c) {
	// Check that filesystem is mounted
	if (!FS_mounted) {
		return ERR_NOT_MOUNTED;
	}
	// Find inode correponsponding to path
	int inode = direntv6_dirlookup(&u, ROOT_INUMBER, c[0]);
	if (inode < 0) return inode;

	// Create a new filev6, and set it's memory to a default value
	struct filev6 stv6;
	memset(&stv6, 255, sizeof(stv6));

	// try to open the file, if we can't => error, otherwise call print_sha_inode
	int inodeOpen = filev6_open(&u, inode, &stv6);
	if(inodeOpen < 0) return inodeOpen;
	print_sha_inode(&u, stv6.i_node, inode);
	return 0;
}
