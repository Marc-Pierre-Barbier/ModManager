
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include "install.h"
#include "main.h"

//TODO: replace ALL system call by execv / execl since it make my code into swiss cheese

void unzip(const char * path, const char * outdir) {
    char * unzipCommand = g_strconcat("/bin/sh -c 'yes | unzip \"", path, "\" -d \"" , outdir, "\" > /dev/null'", NULL);
    if(system(unzipCommand) != 0) {
        printf("\nFailed to unzip archive\n");
        exit(EXIT_FAILURE);
    }
    free(unzipCommand);

}

void unrar(const char * path, const char * outdir) {
    char * unrarCommand = g_strconcat("/bin/sh -c 'unrar -y x \"", path, "\" \"", outdir, "\" > /dev/null'", NULL);
    if(system(unrarCommand) != 0) {
        printf("Failed to unzip archive\n");
        exit(EXIT_FAILURE);
    }
    free(unrarCommand);
}

void un7z(const char * path, const char * outdir) {
    char * un7zCommand = g_strconcat("/bin/sh -c 'yes | 7z x \"", path, "\" \"-o", outdir, "\" > /dev/null'", NULL);
    if(system(un7zCommand) != 0) {
        printf("Failed to unzip archive\n");
        exit(EXIT_FAILURE);
    }
    free(un7zCommand);
}

char * extractLastPart(const char * filePath, const char delimeter) {
int length = strlen(filePath);
    int index = -1;
    for(int i= length -1; i >= 0; i--) {
        if(filePath[i] == delimeter) {
            index = i + 1;
            break;
        }
    }

    if(index <= 0) return NULL;
    char * extension = malloc((length - index) * sizeof(char));
    memcpy(extension, &filePath[index], (length - index));
    return extension;
}

char * extractExtension(const char * filePath) {
    return extractLastPart(filePath, '.');
}

char * extractFileName(const char * filePath) {
    return extractLastPart(filePath, '/');
}


void addMod(const char * filePath, int appId) {
	if (access(filePath, F_OK) != 0) {
        printf("File not found\n");
        exit(EXIT_FAILURE);
	}

    char * configFolder = g_build_filename(getenv("HOME"), MANAGER_FILES, NULL);
	if(g_mkdir_with_parents(configFolder, 0755) < 0) {
        printf("Could not create mod folder");
        exit(EXIT_FAILURE);
    }

    char * filename = extractFileName(filePath);
    char * extension = extractExtension(filename);
    char * lowercaseExtension = g_ascii_strdown(extension, -1);
    free(extension);

    char appIdStr[20];
    sprintf(appIdStr, "%d", appId);

    char * outdir = g_build_filename(configFolder, MOD_FOLDER_NAME, appIdStr, filename, NULL);

    g_mkdir_with_parents(outdir, 0755);
    printf("Adding mod, this process can be slow depending on your hardware\n");
    if(strcmp(lowercaseExtension, "rar") == 0) {
        unrar(filePath, outdir);
    } else if (strcmp(lowercaseExtension, "zip") == 0) {
        unzip(filePath, outdir);
    } else if (strcmp(lowercaseExtension, "7z") == 0) {
        un7z(filePath, outdir);
    } else {
        printf("Unsupported format only zip/7z/rar are supported\n");
        exit(EXIT_FAILURE);
    }

    printf("Done\n");

    free(lowercaseExtension);
    free(filename);
    free(outdir);
}
