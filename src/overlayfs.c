#include <sys/mount.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <glib.h>
#include "overlayfs.h"
#include <stdio.h>

enum overlayErrors overlayMount(char ** sources, const char * dest, const char * upperdir, const char * workdir) {
	char * lowerdir = g_strjoinv(":", sources);
	char * mountData = g_strjoin("", "lowerdir=", lowerdir, ",workdir=", workdir, ",upperdir=", upperdir, NULL);

	enum overlayErrors result = SUCESS;

	if(access("/usr/bin/fuse-overlayfs", F_OK) == 0) {
		int pid = fork();
		if(pid == 0) {
			//exit is used to get the return value when using waitpid
			exit(execl("/usr/bin/fuse-overlayfs", "/usr/bin/fuse-overlayfs", "-r", "-o", mountData, dest, NULL));
		} else {

			int returnValue = 0;
			waitpid(pid, &returnValue, 0);
			if(returnValue != 0) {
				result = FAILURE;
			}
		}
	} else {
		result = NOT_INSTALLED;
	}
	free(lowerdir);
	free(mountData);
	return result;
}
