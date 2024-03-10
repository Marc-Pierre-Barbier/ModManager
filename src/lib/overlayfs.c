#include <sys/mount.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <glib.h>
#include "overlayfs.h"
#include <stdio.h>

overlay_errors_t overlay_mount(char ** sources, const char * dest, const char * upperdir, const char * workdir) {
	g_autofree char * lower_dir = g_strjoinv(":", sources);
	g_autofree char * mount_data = g_strjoin("", "lowerdir=", lower_dir, ",workdir=", workdir, ",upperdir=", upperdir, NULL);

	overlay_errors_t result = SUCCESS;

	if(access("/usr/bin/fuse-overlayfs", F_OK) == 0) {
		int pid = fork();
		if(pid == 0) {
			//exit is used to get the return value when using waitpid
			printf("/usr/bin/fuse-overlayfs -o %s %s\n", mount_data, dest);
			exit(execl("/usr/bin/fuse-overlayfs", "/usr/bin/fuse-overlayfs", "-o", mount_data, dest, NULL));
		} else {

			int return_value = 0;
			waitpid(pid, &return_value, 0);
			if(return_value != 0) {
				result = FAILURE;
			}
		}
	} else {
		result = NOT_INSTALLED;
	}
	return result;
}
