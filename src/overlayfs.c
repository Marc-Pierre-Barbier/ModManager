#include <sys/mount.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <glib.h>
#include "overlayfs.h"
#include <stdio.h>

int overlayMount(char ** sources, const char * dest, const char * upperdir, const char * workdir) {
	char * lowerdir = g_strjoinv(":", sources);
	char * mountData = g_strjoin("", "lowerdir=", lowerdir, ",workdir=", workdir, ",upperdir=", upperdir, NULL);
	int result = mount("overlay", "none", "overlay", MS_RDONLY, mountData);
	if(result < 0) {
		//this is very important for a lot of filesystems
		//even ext4 can be incompatible with the kernel's implementation
		//with some flags such as casefold
		if(access("/usr/bin/fuse-overlayfs", F_OK) == 0) {

			int pid = fork();
			if(pid == 0) {
				//exit is used to get the return value when using waitpid
				exit(execl("/usr/bin/fuse-overlayfs", "/usr/bin/fuse-overlayfs", "-r", "-o", mountData, dest, NULL));
			} else {

				int returnValue = 0;
				waitpid(pid, &returnValue, 0);
				free(lowerdir);
				free(mountData);
				if(returnValue != 0) {
					return -2;
				}
				return 0;
			}

		} else {
			free(lowerdir);
			free(mountData);
			return -1;
		}
	}
	free(lowerdir);
	free(mountData);
	return 0;
}
