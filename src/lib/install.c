
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>

#include <install.h>

#include <constants.h>
#include "archives.h"
#include "file.h"

//TODO: Replace print with errors
error_t install_addMod(char * filePath, int appId) {
	error_t resultError = ERR_SUCCESS;
	if (access(filePath, F_OK) != 0) {
		fprintf(stderr, "File not found\n");
		resultError = ERR_FAILURE;
		goto exit;
	}

	char * configFolder = g_build_filename(getenv("HOME"), MODLIB_WORKING_DIR, NULL);
	if(g_mkdir_with_parents(configFolder, 0755) < 0) {
		fprintf(stderr, "Could not create mod folder\n");
		resultError = ERR_FAILURE;
		goto exit2;
	}

	const char * filename = file_extractFileName(filePath);
	const char * extension = file_extractExtension(filename);
	char * lowercaseExtension = g_ascii_strdown(extension, -1);

	char appIdStr[20];
	sprintf(appIdStr, "%d", appId);
	char * outdir = g_build_filename(configFolder, MOD_FOLDER_NAME, appIdStr, filename, NULL);

	g_mkdir_with_parents(outdir, 0755);

	int returnValue = EXIT_SUCCESS;
	printf("Adding mod, this process can be slow depending on your hardware\n");
	if(strcmp(lowercaseExtension, "rar") == 0) {
		returnValue = archive_unrar(filePath, outdir);
	} else if (strcmp(lowercaseExtension, "zip") == 0) {
		returnValue = archive_unzip(filePath, outdir);
	} else if (strcmp(lowercaseExtension, "7z") == 0) {
		returnValue = archive_un7z(filePath, outdir);
	} else {
		fprintf(stderr, "Unsupported format only zip/7z/rar are supported\n");
		returnValue = EXIT_FAILURE;
	}

	if(returnValue == EXIT_FAILURE) {
		printf("Failed to install mod\n");
		file_delete(outdir, true);
		resultError = ERR_FAILURE;
	}
	else
		printf("Done\n");

	free(lowercaseExtension);
	free(outdir);
exit2:
	free(configFolder);
exit:
	return resultError;
}
