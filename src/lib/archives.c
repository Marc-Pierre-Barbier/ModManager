#include "archives.h"
#include "file.h"

#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>

int archive_unzip(GFile * file, GFile * out_dir) {
	g_autofree char * path = g_file_get_path(file);
	g_autofree char * out_dir_path = g_file_get_path(out_dir);

	char * const args[] = {
		"unzip",
		"-LL", // to lowercase
		"-q",
		path,
		"-d",
		out_dir_path,
		NULL
	};

	pid_t pid = fork();

	if(pid == 0) {
		execv("/usr/bin/unzip", args);
		return EXIT_FAILURE;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);

		if(returnValue != 0) {
			g_error( "\nFailed to decompress archive\n");
		}
		return returnValue;
	}
}

int archive_unrar(GFile * file, GFile * out_dir) {
	g_autofree char * path = g_file_get_path(file);
	g_autofree char * out_dir_path = g_file_get_path(out_dir);
	char * const args[] = {
		"unrar",
		"x",
		"-y", //assume yes
		"-cl", // to lowercase
		path,
		out_dir_path,
		NULL
	};

	pid_t pid = fork();

	if(pid == 0) {
		execv("/usr/bin/unrar", args);
		return EXIT_FAILURE;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);

		if(returnValue != 0) {
			g_error( "\nFailed to decompress archive\n");
		}
		return returnValue;
	}
}


int archive_un7z(GFile * file, GFile * out_dir) {
	g_autofree gchar * outParameter = g_strjoin("", "-o", out_dir, NULL);
	g_autofree char * path = g_file_get_path(file);

	char * const args[] = {
		"7z",
		"-y", //assume yes
		"x",
		path,
		outParameter,
		NULL
	};

	pid_t pid = fork();

	if(pid == 0) {
		execv("/usr/bin/7z", args);
		return EXIT_FAILURE;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);

		if(returnValue != 0) {
			g_error( "\nFailed to decompress archive\n");
			return returnValue;
		}
		//make everything lowercase since 7z don't have an argument for that.
		file_casefold(out_dir);
		return returnValue;
	}
}
