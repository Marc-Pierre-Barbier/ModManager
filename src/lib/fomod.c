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

#include <fomod.h>
#include "file.h"
#include "libxml/globals.h"
#include <constants.h>

#include "fomod/group.h"
#include "fomod/xmlUtil.h"

static gint priorityCmp(gconstpointer a, gconstpointer b) {
	const fomod_File_t * fileA = (const fomod_File_t *)a;
	const fomod_File_t * fileB = (const fomod_File_t *)b;

	return fileB->priority - fileA->priority;
}

//match the definirion of gcompare func
static gint fomod_flagEqual(const fomod_Flag_t * a, const fomod_Flag_t * b) {
	int nameCmp = strcmp(a->name, b->name);
	if(nameCmp == 0) {
		if(strcmp(a->value, b->value) == 0)
			//is equal
			return 0;

		return 1;
	}
	return nameCmp;
}

//TODO: handle error
error_t fomod_processFileOperations(GList ** pendingFileOperations, const char * modFolder, const char * destination) {
	//priority higher a less important and should be processed first.
	*pendingFileOperations = g_list_sort(*pendingFileOperations, priorityCmp);
	GList * currentFileOperation = *pendingFileOperations;

	while(currentFileOperation != NULL) {
		//TODO: support destination
		//no using link since priority is made to override files and link is annoying to deal with when overriding files.
		const fomod_File_t * file = (const fomod_File_t *)currentFileOperation->data;
		char * source = g_build_path("/", modFolder, file->source, NULL);

		//fix the / and \ windows - unix paths
		xml_fixPath(source);

		int copyResult;
		if(file->isFolder) {
			copyResult = file_copy(source, destination, FILE_CP_NO_TARGET_DIR | FILE_CP_RECURSIVE);
		} else {
			copyResult = file_copy(source, destination, 0);
		}
		if(copyResult != EXIT_SUCCESS) {
			fprintf(stderr, "Copy failed, some file might be corrupted\n");
		}
		g_free(source);

		currentFileOperation = g_list_next(currentFileOperation);
	}
	return ERR_SUCCESS;
}

GList * fomod_processCondFiles(const FOMod_t * fomod, GList * flagList, GList * pendingFileOperations) {
	for(int condId = 0; condId < fomod->condFilesCount; condId++) {
		const fomod_CondFile_t *condFile = &fomod->condFiles[condId];

		bool areAllFlagsValid = true;

		//checking if all flags are valid
		for(long flagId = 0; flagId < condFile->flagCount; flagId++) {
			const GList * link = g_list_find_custom(flagList, &(condFile->requiredFlags[flagId]), (GCompareFunc)fomod_flagEqual);
			if(link == NULL) {
				areAllFlagsValid = false;
				break;
			}
		}

		if(areAllFlagsValid) {
			for(long fileId = 0; fileId < condFile->flagCount; fileId++) {
				const fomod_File_t * file = &(condFile->files[fileId]);

				fomod_File_t * fileCopy = g_malloc(sizeof(*file));
				*fileCopy = *file;

				//changing pathes to lowercase since we used casefold and the pathes in the xml might not like it
				char * destination = g_ascii_strdown(file->destination, -1);
				fileCopy->destination = destination;

				char * source = g_ascii_strdown(file->source, -1);
				fileCopy->source = source;

				pendingFileOperations = g_list_append(pendingFileOperations, fileCopy);
			}
		}
	}
	return pendingFileOperations;
}

void fomod_freeFileOperations(GList * fileOperations) {
	GList * fileOperationsStart = fileOperations;
	while(fileOperations != NULL) {
		fomod_File_t * file = (fomod_File_t *)fileOperations->data;
		if(file->destination != NULL)free(file->destination);
		if(file->source != NULL)free(file->source);
		fileOperations = g_list_next(fileOperations);
	}

	g_list_free_full(fileOperationsStart, free);
}

void fomod_freeFOMod(FOMod_t * fomod) {
	for(int i = 0; i < fomod->condFilesCount; i++) {
		fomod_CondFile_t * condFile = &(fomod->condFiles[i]);
		for(long fileId = 0; fileId < condFile->fileCount; fileId++) {
			free(condFile->files[fileId].destination);
			free(condFile->files[fileId].source);
		}

		for(long flagId = 0; flagId < condFile->flagCount; flagId++) {
			fomod_Flag_t * flag = &(condFile->requiredFlags[flagId]);
			free(flag->name);
			free(flag->value);
		}
		free(condFile->files);
		free(condFile->requiredFlags);
	}
	free(fomod->condFiles);
	free(fomod->moduleImage);
	free(fomod->moduleName);

	int size = fomod_countUntilNull(fomod->requiredInstallFiles);
	for(int i = 0; i < size; i++) {
		free(fomod->requiredInstallFiles[i]);
	}
	free(fomod->requiredInstallFiles);

	for(int i = 0; i < fomod->stepCount; i++) {
		FOModStep_t * step = &fomod->steps[i];
		for(int groupId = 0; groupId < step->groupCount; groupId++) {
			fomod_Group_t * group = &step->groups[groupId];
			grp_freeGroup(group);
		}
		for(int flagId = 0; flagId < step->flagCount; flagId++) {
			fomod_Flag_t * flag = &(step->requiredFlags[flagId]);
			free(flag->name);
			free(flag->value);
		}
		free(step->groups);
		free(step->requiredFlags);
		free(step->name);
	}
	free(fomod->steps);

	//set every counter to zero and every pointer to null
	memset(fomod, 0, sizeof(FOMod_t));
}
