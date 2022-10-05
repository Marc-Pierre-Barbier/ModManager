
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>
#include "install.h"
#include "main.h"
#include "file.h"

static int unzip(char * path, char * outdir) {
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

static int unrar(char * path, char * outdir) {
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


static int un7z(char * path, const char * outdir) {
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
		casefold(outdir);
		return returnValue;
	}
}

static const char * extractLastPart(const char * filePath, const char delimeter) {
	const unsigned long length = strlen(filePath);
	long index = -1;
	for(long i= length - 1; i >= 0; i--) {
		if(filePath[i] == delimeter) {
			index = i + 1;
			break;
		}
	}

	if(index <= 0 || index == length) return NULL;
	return &filePath[index];
}

static const char * extractExtension(const char * filePath) {
	return extractLastPart(filePath, '.');
}

static const char * extractFileName(const char * filePath) {
	return extractLastPart(filePath, '/');
}

int addMod(char * filePath, int appId) {
	int returnValue = EXIT_SUCCESS;

	if (access(filePath, F_OK) != 0) {
		fprintf(stderr, "File not found\n");
		returnValue = EXIT_FAILURE;
		goto exit;
	}

	char * configFolder = g_build_filename(getenv("HOME"), MANAGER_FILES, NULL);
	if(g_mkdir_with_parents(configFolder, 0755) < 0) {
		fprintf(stderr, "Could not create mod folder");
		returnValue = EXIT_FAILURE;
		goto exit2;
	}

	const char * filename = extractFileName(filePath);
	const char * extension = extractExtension(filename);
	char * lowercaseExtension = g_ascii_strdown(extension, -1);

	char appIdStr[20];
	sprintf(appIdStr, "%d", appId);
	char * outdir = g_build_filename(configFolder, MOD_FOLDER_NAME, appIdStr, filename, NULL);

	g_mkdir_with_parents(outdir, 0755);
	printf("Adding mod, this process can be slow depending on your hardware\n");
	if(strcmp(lowercaseExtension, "rar") == 0) {
		returnValue = unrar(filePath, outdir);
	} else if (strcmp(lowercaseExtension, "zip") == 0) {
		returnValue = unzip(filePath, outdir);
	} else if (strcmp(lowercaseExtension, "7z") == 0) {
		returnValue = un7z(filePath, outdir);
	} else {
		fprintf(stderr, "Unsupported format only zip/7z/rar are supported\n");
		returnValue = EXIT_FAILURE;
	}

	printf("Done\n");

	free(lowercaseExtension);
	free(outdir);
exit2:
	free(configFolder);
exit:
	return returnValue;
}
