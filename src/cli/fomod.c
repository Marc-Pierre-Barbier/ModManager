#include "fomod.h"
#include <fomod.h>
#include <constants.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

//match the definirion of gcompare func
static gint fomod_flag_equal(const FomodFlag_t * a, const FomodFlag_t * b) {
	int nameCmp = strcmp(a->name, b->name);
	if(nameCmp == 0) {
		if(strcmp(a->value, b->value) == 0)
			//is equal
			return 0;

		return 1;
	}
	return nameCmp;
}

static int getInputCount(const char * input) {
	char buff[2];
	buff[1] = '\0';

	int elementCount = 0;
	bool prevWasValue = false;

	for(int i = 0; input[i] != '\0' && input[i] != '\n'; i++) {
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

static void fomod_printOptionsInOrder(FomodGroup_t group) {
	for(int i = 0; i < group.plugin_count; i++) {
		printf("%d, %s\n", i, group.plugins[i].name);
		printf("%s\n", group.plugins[i].description);
	}
}

error_t fomod_installFOMod(char * mod_folder, GFile * destination) {
	//everything should be lowercase since we use casefold() before calling any install function
	g_autofree GFile * fomod_folder_file = g_file_new_build_filename(mod_folder, "fomod", NULL);
	g_autofree char * fomod_folder = g_file_get_path(fomod_folder_file);
	g_autofree GFile * fomod_file = g_file_new_build_filename(fomod_folder, "moduleconfig.xml", NULL);

	if(!g_file_query_exists(fomod_file, NULL)) {
		g_error( "FOMod file not found, are you sure this is a fomod mod ?\n");
		return ERR_FAILURE;
	}

	Fomod_t fomod;
	int returnValue = fomod_parse(fomod_file, &fomod);
	if(returnValue == ERR_FAILURE)
		return ERR_FAILURE;

	GList * flagList = NULL;
	GList * pendingFileOperations = NULL;

	for(int i = 0; i < fomod.step_count; i++) {
		const FomodStep_t * step = &fomod.steps[i];

		bool validFlags = true;
		for(int flagId = 0; flagId < step->flag_count; flagId++) {
			const GList * flagLink = g_list_find_custom(flagList, &step->required_flags[flagId], (GCompareFunc)fomod_flag_equal);
			if(flagLink == NULL) {
				validFlags = false;
				break;
			}
		}

		if(!validFlags) continue;

		for(int groupId = 0; groupId < step->group_count; groupId++ ) {
			FomodGroup_t group = step->groups[groupId];

			u_int8_t min;
			u_int8_t max;
			int inputCount = 0;
			char *inputBuffer = NULL;
			size_t bufferSize = 0;

			while(true) {
				printf("%s\n", group.name);
				fomod_printOptionsInOrder(group);
				switch(group.type) {
				case ONE_ONLY:
					printf("Select one :\n");
					min = 1;
					max = 1;
					break;

				case ANY:
					printf("Select any (space separated) (leave empty for nothing) :\n");
					min = 0;
					max = (u_int8_t)group.plugin_count;
					break;

				case AT_LEAST_ONE:
					printf("Select at least one (space separated) (leave empty for nothing) :\n");
					min = 1;
					max = (u_int8_t)group.plugin_count;
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
				default:
					//never happen;
					g_error( "unexpected type please report this issue %d, %d", group.type, __LINE__);
					return ERR_FAILURE;
				}



				if(getline(&inputBuffer, &bufferSize, stdin) > 0) {
					inputCount = getInputCount(inputBuffer);
					if(inputCount <= max && inputCount >= min) {
						printf("Continuing\n");
						break;
					}
					g_error( "Invalid input\n");
					if(inputBuffer != NULL)free(inputBuffer);
					inputBuffer = NULL;
				} else {
				//TODO: handle error
				}
			}

			//processing user input

			GRegex* regex = g_regex_new("\\s*", 0, 0, NULL);
			char ** choices = g_regex_split(regex, inputBuffer, 0);
			g_regex_unref(regex);

			for(int choiceId = 0; choices[choiceId] != NULL; choiceId++) {
				//TODO: safer user input
				int choice = atoi(choices[choiceId]);
				FomodPlugin_t plugin = group.plugins[choice];
				for(int flagId = 0; flagId < plugin.flag_count; flagId++) {
					flagList = g_list_append(flagList, &plugin.flags[flagId]);
				}

				//do the install
				for(int pluginId = 0; pluginId < plugin.file_count; pluginId++) {
					const FomodFile_t * file = &plugin.files[pluginId];

					FomodFile_t * fileCopy = g_malloc(sizeof(FomodFile_t));
					*fileCopy = *file;

					//changing pathes to lowercase since we used casefold and the pathes in the xml might not like it
					//char * destination = g_ascii_strdown(file->destination, -1);
					//TODO: replace by GFile
					fileCopy->destination = g_file_get_path(destination);
					//g_free(destination);

					char * source = g_ascii_strdown(file->source, -1);
					fileCopy->source = strdup(source);
					g_free(source);

					pendingFileOperations = g_list_append(pendingFileOperations, fileCopy);
				}
			}

			g_strfreev(choices);
			if(inputBuffer != NULL)free(inputBuffer);
		}
	}

	//TODO: manage multiple files with the same name

	pendingFileOperations = fomod_process_cond_files(&fomod, flagList, pendingFileOperations);
	//fomod_process_file_operations(&pendingFileOperations, source_mod, destination, appid);

	printf("FOMod successfully installed!\n");
	g_list_free(flagList);
	fomod_free_file_operations(pendingFileOperations);
	fomod_free_fomod(&fomod);
	return ERR_SUCCESS;
}
