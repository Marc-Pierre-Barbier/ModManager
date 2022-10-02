
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include "install.h"
#include "main.h"

//TODO: replace ALL system call by execv / execl since it make my code into swiss cheese
// we can imagine a milicious fomod with a simple shell code inside.

int unzip(const char * path, const char * outdir) {
	int returnValue = EXIT_SUCCESS;
	char * unzipCommand = g_strconcat("/bin/sh -c 'yes | unzip \"", path, "\" -d \"" , outdir, "\" > /dev/null'", NULL);
	if(system(unzipCommand) != 0) {
		printf("\nFailed to decompress archive\n");
		returnValue = EXIT_FAILURE;
	}
	free(unzipCommand);
	return returnValue;
}

int unrar(const char * path, const char * outdir) {
	int returnValue = EXIT_SUCCESS;
	char * unrarCommand = g_strconcat("/bin/sh -c 'unrar -y x \"", path, "\" \"", outdir, "\" > /dev/null'", NULL);
	if(system(unrarCommand) != 0) {
		printf("Failed to decompress archive\n");
		returnValue = EXIT_FAILURE;
	}
	free(unrarCommand);
	return returnValue;
}

int un7z(const char * path, const char * outdir) {
	int returnValue = EXIT_SUCCESS;
	char * un7zCommand = g_strconcat("/bin/sh -c 'yes | 7z x \"", path, "\" \"-o", outdir, "\" > /dev/null'", NULL);
	if(system(un7zCommand) != 0) {
		printf("Failed to decompress archive\n");
		returnValue = EXIT_FAILURE;
	}
	free(un7zCommand);
	return returnValue;
}

const char * extractLastPart(const char * filePath, const char delimeter) {
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

const char * extractExtension(const char * filePath) {
	return extractLastPart(filePath, '.');
}

const char * extractFileName(const char * filePath) {
	return extractLastPart(filePath, '/');
}


int addMod(const char * filePath, int appId) {
	int returnValue = EXIT_SUCCESS;

	if (access(filePath, F_OK) != 0) {
		printf("File not found\n");
		returnValue = EXIT_FAILURE;
		goto exit;
	}

	char * configFolder = g_build_filename(getenv("HOME"), MANAGER_FILES, NULL);
	if(g_mkdir_with_parents(configFolder, 0755) < 0) {
		printf("Could not create mod folder");
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
		printf("Unsupported format only zip/7z/rar are supported\n");
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
