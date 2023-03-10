#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <glib.h>
#include "file.h"

static u_int32_t countSetBits(u_int32_t n) {
	// base case
	if (n == 0)
		return 0;
	else
		// if last bit set add 1 else add 0
		return (n & 1) + countSetBits(n >> 1);
}

//TODO: add interruption support
//simplest way to copy a file in c(linux)
int file_copy(const char * path, const char * dest, u_int32_t flags) {
	int flagCount = countSetBits(flags);
	if(flagCount > 3) {
		fprintf(stderr, "Invalid flags for cp command\n");
		return -1;
	}
	//flags + cp + path + dest
	char * args[flagCount + 4];
	args[0] = "/bin/cp";
	args[1] = alloca((strlen(path) + 1) * sizeof(char));
	strcpy(args[1], path);
	args[2] = alloca((strlen(dest) + 1) * sizeof(char));
	strcpy(args[2], dest);
	int argIndex = 3;

	if(flags & FILE_CP_LINK) {
		args[argIndex] = "--link";
		argIndex += 1;
	}
	if(flags & FILE_CP_RECURSIVE) {
		args[argIndex] = "-r";
		argIndex += 1;
	}
	if(flags & FILE_CP_NO_TARGET_DIR) {
		args[argIndex] = "-T";
		argIndex += 1;
	}

	args[argIndex] = NULL;

	int pid = fork();
	if(pid == 0) {
		//discard the const. since we are in a fork we don't care.
		execv("/bin/cp", args);
		return 0;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);
		return returnValue;
	}
}

int file_delete(const char * path, bool recursive) {
	int pid = fork();
	if(pid == 0) {
		if(recursive) {
			execl("/bin/rm", "/bin/rm", "-r", path, NULL);
		} else {
			execl("/bin/rm", "/bin/rm",  path, NULL);
		}
		return 0;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);
		return returnValue;
	}
}

int file_move(const char * source, const char * destination) {
	int pid = fork();
	if(pid == 0) {
		execl("/bin/mv", "/bin/mv", source, destination, NULL);
		return 0;
	} else {
		int returnValue;
		waitpid(pid, &returnValue, 0);
		return returnValue;
	}
}


//rename a folder and all subfolder and files to lowercase
//TODO: error handling
void file_casefold(const char * folder) {
	DIR * d = opendir(folder);
	struct dirent *dir;
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			//removes .. & . from the list
			if(strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {
				//only look at ascii and hope for the best.
				gchar * file = g_build_filename(folder, dir->d_name, NULL);

				gchar * destinationName = g_ascii_strdown(dir->d_name, -1);
				gchar * destination = g_build_path("/", folder,destinationName, NULL);


				if(strcmp(destinationName, dir->d_name) != 0) {

					int result = file_move(file, destination);
					if(result != EXIT_SUCCESS) {
						fprintf(stderr, "Move failed: %s => %s \n", dir->d_name, destinationName);
					}


				}
				g_free(file);
				g_free(destinationName);

				if(dir->d_type == DT_DIR) {
					file_casefold(destination);
				}

				g_free(destination);
			}
		}
		closedir(d);
	}
}

const char * file_extractLastPart(const char * filePath, const char delimeter) {
	const int length = strlen(filePath);
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

const char * file_extractExtension(const char * filePath) {
	return file_extractLastPart(filePath, '.');
}

const char * file_extractFileName(const char * filePath) {
	return file_extractLastPart(filePath, '/');
}

GList * file_listFolderContent(const char * path) {
	GList * list = NULL;
	DIR *d;
	struct dirent *dir;
	d = opendir(path);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			//removes .. & . from the list
			if(strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {
				list = g_list_append(list, strdup(dir->d_name));
			}
		}
		closedir(d);
	}

	return list;
}
