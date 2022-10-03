#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <unistd.h>

#include "fomod.h"
#include "file.h"
#include "libxml/globals.h"

int getInputCount(const char * input) {
	char buff[2];
	buff[1] = '\0';

	int elementCount = 0;
	bool prevWasValue = false;

	for(int i = 0; input + i != NULL && input[i] != '\0' && input[i] != '\n'; i++) {
		buff[0] = input[i];

		//if the character is a valid character
		if(strstr("0123456789 ", buff) != NULL) {
			if(input[i] == ' ') prevWasValue = false;
			else if(!prevWasValue) {
				prevWasValue = true;
				elementCount++;
			}
		} else {
			return -1;
		}

	}

	return elementCount;
}

gint priorityCmp(gconstpointer a, gconstpointer b) {
	const FOModFile_t * fileA = (const FOModFile_t *)a;
	const FOModFile_t * fileB = (const FOModFile_t *)b;

	return fileA->priority - fileB->priority;
}

void printfOptionsInOrder(FOModGroup_t group) {
	for(int i = 0; i < group.pluginCount; i++) {
		printf("%d, %s\n", i, group.plugins[i].name);
		printf("%s\n", group.plugins[i].description);
	}
}

gint flagEqual(const FOModFlag_t * a, const FOModFlag_t * b) {
	int nameCmp = strcmp(a->name, b->name);
	if(nameCmp == 0) {
		if(strcmp(a->value, b->value) == 0)
			//is equal
			return 0;

		return 1;
	}
	return nameCmp;
}

int stepCmpAsc(const void * stepA, const void * stepB) {
	const FOModStep_t * step1 = (const FOModStep_t *)stepA;
	const FOModStep_t * step2 = (const FOModStep_t *)stepB;
	return strcmp(step1->name, step2->name);
}

int stepCmpDesc(const void * stepA, const void * stepB) {
	const FOModStep_t * step1 = (const FOModStep_t *)stepA;
	const FOModStep_t * step2 = (const FOModStep_t *)stepB;
	return 1 - strcmp(step1->name, step2->name);
}


void sortSteps(FOMod_t * fomod) {
	switch(fomod->stepOrder) {
	case ASC:
		qsort(fomod->steps, fomod->stepCount, sizeof(*fomod->steps), stepCmpAsc);
		break;
	case DESC:
		qsort(fomod->steps, fomod->stepCount, sizeof(*fomod->steps), stepCmpDesc);
		break;
	case ORD:
		//ord mean that we keep the curent order, so no need to sort anything.
		break;
	}
}

int groupCmpAsc(const void * stepA, const void * stepB) {
	const FOModGroup_t * step1 = (const FOModGroup_t *)stepA;
	const FOModGroup_t * step2 = (const FOModGroup_t *)stepB;
	return strcmp(step1->name, step2->name);
}

int groupCmpDesc(const void * stepA, const void * stepB) {
	const FOModGroup_t * step1 = (const FOModGroup_t *)stepA;
	const FOModGroup_t * step2 = (const FOModGroup_t *)stepB;
	return 1 - strcmp(step1->name, step2->name);
}

void sortGroup(FOModGroup_t * group) {
	switch(group->order) {
	case ASC:
		qsort(group->plugins, group->pluginCount, sizeof(*group->plugins), groupCmpAsc);
		break;
	case DESC:
		qsort(group->plugins, group->pluginCount, sizeof(*group->plugins), groupCmpDesc);
		break;
	case ORD:
		//ord mean that we keep the curent order, so no need to sort anything.
		break;
	}
}

//TODO: handle error
int processFileOperations(GList * pendingFileOperations, const char * modFolder, const char * destination) {
	pendingFileOperations = g_list_sort(pendingFileOperations, priorityCmp);
	GList * currentFileOperation = pendingFileOperations;

	while(currentFileOperation != NULL) {
		//TODO: support destination
		//no using link since priority is made to override files and link is annoying to deal with when overriding files.
		const FOModFile_t * file = (const FOModFile_t *)currentFileOperation->data;
		char * source = g_build_path("/", modFolder, file->source, NULL);

		//fix the / and \ windows - unix paths
		fixPath(source);

		int copyResult;
		if(file->isFolder) {
			copyResult = copy(source, destination, CP_NO_TARGET_DIR | CP_RECURSIVE);
		} else {
			copyResult = copy(source, destination, 0);
		}
		if(copyResult != EXIT_SUCCESS) {
			fprintf(stderr, "Copy failed, some file might be corrupted\n");
		}
		printf("source: %s, destination: %s\n", source, destination);
		g_free(source);

		currentFileOperation = g_list_next(currentFileOperation);
	}
	return EXIT_SUCCESS;
}

GList * processCondFiles(const FOMod_t * fomod, GList * flagList, GList * pendingFileOperations) {
	for(int condId = 0; condId < fomod->condFilesCount; condId++) {
		const FOModCondFile_t *condFile = &fomod->condFiles[condId];

		bool areAllFlagsValid = true;

		//checking if all flags are valid
		for(int flagId = 0; flagId < condFile->flagCount; flagId++) {
			const GList * link = g_list_find_custom(flagList, &(condFile->requiredFlags[flagId]), (GCompareFunc)flagEqual);
			if(link == NULL) {
				areAllFlagsValid = false;
				break;
			}
		}

		if(areAllFlagsValid) {
			for(int fileId = 0; fileId < condFile->flagCount; fileId++) {
				const FOModFile_t * file = &(condFile->files[fileId]);

				FOModFile_t * fileCopy = malloc(sizeof(*file));
				*fileCopy = *file;

				pendingFileOperations = g_list_append(pendingFileOperations, fileCopy);
			}
		}
	}
	return pendingFileOperations;
}

int installFOMod(const char * modFolder, const char * destination) {
	//everything should be lowercase since we use casefold() before calling any install function
	char * fomodFolder = g_build_path("/", modFolder, "fomod", NULL);
	char * fomodFile = g_build_filename(fomodFolder, "moduleconfig.xml", NULL);

	if(access(fomodFile, F_OK) != 0) {
		fprintf(stderr, "FOMod file not found, are you sure this is a fomod mod ?\n");
		g_free(fomodFolder);
		g_free(fomodFile);
		return EXIT_FAILURE;
	}

	FOMod_t fomod;
	int returnValue = parseFOMod(fomodFile, &fomod);
	if(returnValue != EXIT_SUCCESS) {
		return returnValue;
	}
	g_free(fomodFile);

	GList * flagList = NULL;
	GList * pendingFileOperations = NULL;

	sortSteps(&fomod);

	for(int i = 0; i < fomod.stepCount; i++) {
		const FOModStep_t * step = &fomod.steps[i];

		bool validFlags = true;
		for(int flagId = 0; flagId < step->flagCount; flagId++) {
			const GList * flagLink = g_list_find_custom(flagList, &step->requiredFlags[flagId], (GCompareFunc)flagEqual);
			if(flagLink == NULL) {
				validFlags = false;
				break;
			}
		}

		if(!validFlags) continue;

		for(int groupId = 0; groupId < step->groupCount; groupId++ ) {
			FOModGroup_t group = step->groups[groupId];

			sortGroup(&group);

			u_int8_t min;
			u_int8_t max;
			int inputCount = 0;
			char *inputBuffer = NULL;
			size_t bufferSize = 0;

			while(true) {
				printfOptionsInOrder(group);
				switch(group.type) {
				case ONE_ONLY:
					printf("Select one :\n");
					min = 1;
					max = 1;
					break;

				case ANY:
					printf("Select any (space separated) (leave empty for nothing) :\n");
					min = 0;
					max = (u_int8_t)group.pluginCount - 1;
					break;

				case AT_LEAST_ONE:
					printf("Select at least one (space separated) (leave empty for nothing) :\n");
					min = 1;
					max = (u_int8_t)group.pluginCount - 1;
					break;

				case AT_MOST_ONE:
					printf("Select one or none (space separated) (leave empty for nothing) :\n");
					min = 0;
					max = 1;
					break;

				case ALL:
					printf("Press enter to continue\n");
					min = 0;
					max = 0;
					break;
				}



				if(getline(&inputBuffer, &bufferSize, stdin) > 0) {
					inputCount = getInputCount(inputBuffer);
					if(inputCount <= max && inputCount >= min) {
						printf("Continuing\n");
						break;
					}
					fprintf(stderr, "Invalid input\n");
					if(inputBuffer != NULL)free(inputBuffer);
					inputBuffer = NULL;
				} else {
				//TODO: handle error
				}
			}

			GRegex* regex = g_regex_new("\\s*", 0, 0, NULL);
			char ** choices = g_regex_split(regex, inputBuffer, 0);
			g_regex_unref(regex);

			for(int choiceId = 0; choices[choiceId] != NULL; choiceId++) {
				int choice = atoi(choices[choiceId]);
				FOModPlugin_t plugin = group.plugins[choice];
				for(int flagId = 0; flagId < plugin.flagCount; flagId++) {
					flagList = g_list_append(flagList, &plugin.flags[flagId]);
				}

				//do the install
				for(int pluginId = 0; pluginId < plugin.fileCount; pluginId++) {
					const FOModFile_t * file = &plugin.files[pluginId];

					FOModFile_t * fileCopy = malloc(sizeof(*file));
					*fileCopy = *file;

					pendingFileOperations = g_list_append(pendingFileOperations, fileCopy);
				}
			}

			g_strfreev(choices);
			if(inputBuffer != NULL)free(inputBuffer);
		}
	}



	pendingFileOperations = processCondFiles(&fomod, flagList, pendingFileOperations);
	processFileOperations(pendingFileOperations, modFolder, destination);

	printf("FOMod successfully installed!\n");
	g_list_free(flagList);
	g_list_free_full(pendingFileOperations, free);
	freeFOMod(&fomod);
	g_free(fomodFolder);
	return EXIT_SUCCESS;
}
