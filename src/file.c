#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "file.h"
#include <stdio.h>

u_int32_t countSetBits(u_int32_t n) {
    // base case
    if (n == 0)
        return 0;
    else
        // if last bit set add 1 else add 0
        return (n & 1) + countSetBits(n >> 1);
}

//TODO: add interruption support
//simplest way to copy a file in c(linux)
int copy(const char * path, const char * dest, u_int32_t flags) {
	int flagCount = countSetBits(flags);
	if(flagCount > 3) {
		printf("Invalid flags for cp command\n");
		return -1;
	}
	//flags + cp + path + dest
	const char * args[flagCount + 4];
	args[0] = "/bin/cp";
	args[1] = path;
	args[2] = dest;
	int argIndex = 3;

	if(flags & CP_LINK) {
		args[argIndex] = "--link";
		argIndex += 1;
	}
	if(flags & CP_RECURSIVE) {
		args[argIndex] = "-r";
		argIndex += 1;
	}
	if(flags & CP_NO_TARGET_DIR) {
		args[argIndex] = "-T";
		argIndex += 1;
	}

	args[argIndex] = NULL;


	int pid = fork();
	if(pid == 0) {
		//discard the const. since we are in a fork we don't care.
		execv("/bin/cp", (char **)args);
		return 0;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);
		return returnValue;
	}
}

int delete(const char * path, bool recursive) {
	int pid = fork();
	if(pid == 0) {
		if(recursive) {
			execl("/bin/rm", "/bin/rm", "-r", path, NULL);
		} else {
			execl("/bin/rm", "/bin/rm",  path, NULL);
		}
		return 0;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);
		return returnValue;
	}
}

int move(const char * source, const char * destination) {
	int pid = fork();
	if(pid == 0) {
		execl("/bin/mv", "/bin/mv", source, destination, NULL);
		return 0;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);
		return returnValue;
	}
}
