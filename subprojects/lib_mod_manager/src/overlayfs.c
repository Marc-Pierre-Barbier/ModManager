#include <sys/mount.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <glib.h>
#include "overlayfs.h"
#include <stdio.h>

overlay_errors_t overlay_mount(char ** sources, const char * dest, const char * upperdir) {
	g_autofree char * lower_dirs = g_strjoinv(":", sources);
	g_autofree char * mount_data = g_strjoin("", "--lowerdirs=\"", lower_dirs, "\" --upperdir=\"", upperdir, "\"", NULL);
	g_autofree char * full_command = g_strjoin(" ", "modfs", mount_data, dest, NULL);

	overlay_errors_t result = SUCCESS;

	if(system("which modfs") == 0) {
		int return_value = system(full_command);
		if(return_value != 0) {
			result = FAILURE;
		}
	} else {
		result = NOT_INSTALLED;
	}
	return result;
}
