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

static gint priority_cmp(gconstpointer a, gconstpointer b) {
	const Fomod_File_t * fileA = (const Fomod_File_t *)a;
	const Fomod_File_t * fileB = (const Fomod_File_t *)b;

	return fileB->priority - fileA->priority;
}

//match the definirion of gcompare func
static gint fomod_flagEqual(const Fomod_Flag_t * a, const Fomod_Flag_t * b) {
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
error_t fomod_process_file_operations(GList ** pending_file_operations, GFile * modFolder, GFile* destination) {
	//priority higher a less important and should be processed first.
	*pending_file_operations = g_list_sort(*pending_file_operations, priority_cmp);
	GList * file_operation_iterator = *pending_file_operations;

	while(file_operation_iterator != NULL) {
		//TODO: support destination
		//no using link since priority is made to override files and link is annoying to deal with when overriding files.
		const Fomod_File_t * file = (const Fomod_File_t *)file_operation_iterator->data;

		////fix the / and \ windows - unix paths
		xml_fixPath(file->source);

		char * mod_folder_path = g_file_get_path(modFolder);
		//TODO: check if it can build from 2 path
		GFile * source = g_file_new_build_filename(mod_folder_path, file->source, NULL);
		g_free(mod_folder_path);

		int copyResult = EXIT_SUCCESS;
		if(file->isFolder) {
			copyResult = file_recursive_copy(source, destination, G_FILE_COPY_NONE, NULL, NULL);
		} else {
			if(!g_file_copy(source, destination, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL))
				copyResult = EXIT_FAILURE;
		}
		if(copyResult != EXIT_SUCCESS) {
			g_error( "Copy failed, some file might be corrupted\n");
		}
		g_free(source);

		file_operation_iterator = g_list_next(file_operation_iterator);
	}
	return ERR_SUCCESS;
}

GList * fomod_process_cond_files(const Fomod_t * fomod, GList * flagList, GList * pendingFileOperations) {
	for(int condId = 0; condId < fomod->condFilesCount; condId++) {
		const Fomod_CondFile_t *condFile = &fomod->condFiles[condId];

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
				const Fomod_File_t * file = &(condFile->files[fileId]);

				Fomod_File_t * fileCopy = g_malloc(sizeof(*file));
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

void fomod_free_file_operations(GList * fileOperations) {
	GList * fileOperationsStart = fileOperations;
	while(fileOperations != NULL) {
		Fomod_File_t * file = (Fomod_File_t *)fileOperations->data;
		if(file->destination != NULL)free(file->destination);
		if(file->source != NULL)free(file->source);
		fileOperations = g_list_next(fileOperations);
	}

	g_list_free_full(fileOperationsStart, free);
}

void fomod_free_fomod(Fomod_t * fomod) {
	for(int i = 0; i < fomod->condFilesCount; i++) {
		Fomod_CondFile_t * condFile = &(fomod->condFiles[i]);
		for(long fileId = 0; fileId < condFile->fileCount; fileId++) {
			free(condFile->files[fileId].destination);
			free(condFile->files[fileId].source);
		}

		for(long flagId = 0; flagId < condFile->flagCount; flagId++) {
			Fomod_Flag_t * flag = &(condFile->requiredFlags[flagId]);
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
		FomodStep_t * step = &fomod->steps[i];
		for(int groupId = 0; groupId < step->groupCount; groupId++) {
			fomodGroup_t * group = &step->groups[groupId];
			grp_freeGroup(group);
		}
		for(int flagId = 0; flagId < step->flagCount; flagId++) {
			Fomod_Flag_t * flag = &(step->requiredFlags[flagId]);
			free(flag->name);
			free(flag->value);
		}
		free(step->groups);
		free(step->requiredFlags);
		free(step->name);
	}
	free(fomod->steps);

	//set every counter to zero and every pointer to null
	memset(fomod, 0, sizeof(Fomod_t));
}
