#include "archives.h"
#include "file.h"

#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>

int archive_unzip(char * path, char * outdir) {
	char * const args[] = {
		"unzip",
		"-LL", // to lowercase
		"-q",
		path,
		"-d",
		outdir,
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
			fprintf(stderr, "\nFailed to decompress archive\n");
		}
		return returnValue;
	}
}

int archive_unrar(char * path, char * outdir) {
	char * const args[] = {
		"unrar",
		"x",
		"-y", //assume yes
		"-cl", // to lowercase
		path,
		outdir,
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
			fprintf(stderr, "\nFailed to decompress archive\n");
		}
		return returnValue;
	}
}


int archive_un7z(char * path, const char * outdir) {
	gchar * outParameter = g_strjoin("", "-o", outdir, NULL);

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
		g_free(outParameter);
		int returnValue;
		waitpid(pid, &returnValue, 0);

		if(returnValue != 0) {
			fprintf(stderr, "\nFailed to decompress archive\n");
			return returnValue;
		}
		//make everything lowercase since 7z don't have an argument for that.
		file_casefold(outdir);
		return returnValue;
	}
}
