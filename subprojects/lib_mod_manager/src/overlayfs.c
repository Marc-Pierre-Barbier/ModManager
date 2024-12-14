#include <sys/mount.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <glib.h>
#include "overlayfs.h"
#include <stdio.h>

overlay_errors_t overlay_mount(char ** sources, const char * dest, const char * upperdir, const char * workdir) {
	g_autofree char * lower_dir = g_strjoinv("\":\"", sources);
	g_autofree char * mount_data = g_strjoin("", "lowerdir=\"", lower_dir, "\",workdir=\"", workdir, "\",upperdir=\"", upperdir, "\"", NULL);
	g_autofree char * full_command = g_strjoin(" ", "fuse-overlayfs", "-o", mount_data, dest, NULL);

	overlay_errors_t result = SUCCESS;

	printf("%s\n", dest);

	if(access("/usr/bin/fuse-overlayfs", F_OK) == 0) {
		int return_value = system(full_command);
		if(return_value != 0) {
			result = FAILURE;
		}
	} else {
		result = NOT_INSTALLED;
	}
	return result;
}
