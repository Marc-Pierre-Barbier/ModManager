
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>

#include "install.h"
#include "main.h"
#include "archives.h"
#include "file.h"

error_t addMod(char * filePath, int appId) {
	error_t resultError = ERR_SUCCESS;
	if (access(filePath, F_OK) != 0) {
		fprintf(stderr, "File not found\n");
		resultError = ERR_FAILURE;
		goto exit;
	}

	char * configFolder = g_build_filename(getenv("HOME"), MANAGER_FILES, NULL);
	if(g_mkdir_with_parents(configFolder, 0755) < 0) {
		fprintf(stderr, "Could not create mod folder");
		resultError = ERR_FAILURE;
		goto exit2;
	}

	const char * filename = extractFileName(filePath);
	const char * extension = extractExtension(filename);
	char * lowercaseExtension = g_ascii_strdown(extension, -1);

	char appIdStr[20];
	sprintf(appIdStr, "%d", appId);
	char * outdir = g_build_filename(configFolder, MOD_FOLDER_NAME, appIdStr, filename, NULL);

	g_mkdir_with_parents(outdir, 0755);

	int returnValue = EXIT_SUCCESS;
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

	if(returnValue == EXIT_FAILURE)
		resultError = ERR_FAILURE;

	printf("Done\n");

	free(lowercaseExtension);
	free(outdir);
exit2:
	free(configFolder);
exit:
	return resultError;
}
