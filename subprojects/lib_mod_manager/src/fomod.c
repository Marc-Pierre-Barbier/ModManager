#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

#include <fomod.h>
#include "file.h"
#include "libxml/globals.h"
#include <constants.h>

#include "fomod/group.h"
#include "fomod/xmlUtil.h"
#include "mods.h"
#include "steam.h"

static gint priority_cmp(gconstpointer a, gconstpointer b) {
	const FomodFile_t * file_a = (const FomodFile_t *)a;
	const FomodFile_t * file_b = (const FomodFile_t *)b;

	return file_b->priority - file_a->priority;
}

//match the definirion of gcompare func
static gint fomod_flag_equal(const FomodFlag_t * a, const FomodFlag_t * b) {
	int name_cmp = strcmp(a->name, b->name);
	if(name_cmp == 0) {
		if(strcmp(a->value, b->value) == 0)
			//is equal
			return 0;

		return 1;
	}
	return name_cmp;
}

//TODO: handle error
error_t fomod_process_file_operations(GList ** pending_file_operations, int mod_id, int appid) {
	char app_id_str[GAMES_MAX_APPID_LENGTH];
	snprintf(app_id_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	GList * mods = mods_list(appid);
	const char * souce_name = g_list_nth(mods, mod_id)->data;
	const char * home = g_get_home_dir();
	g_autofree const char * mods_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, app_id_str, NULL);
	g_autofree  GFile * mod_folder = g_file_new_build_filename(mods_folder, souce_name, NULL);
	g_autofree const char * destination_name = g_strconcat(souce_name, "__FOMOD", NULL);
	g_autofree const char * destination_folder = g_build_filename(mods_folder, destination_name, NULL);

	//priority higher a less important and should be processed first.
	*pending_file_operations = g_list_sort(*pending_file_operations, priority_cmp);
	GList * file_operation_iterator = *pending_file_operations;
	g_autofree char * mod_folder_path = g_file_get_path(mod_folder);
	mkdir(destination_folder, 0700);

	while(file_operation_iterator != NULL) {
		//TODO: support destination
		//no using link since priority is made to override files and link is annoying to deal with when overriding files.
		const FomodFile_t * file = (const FomodFile_t *)file_operation_iterator->data;

		////fix the / and \ windows - unix paths
		xml_fix_path(file->source);

		//TODO: check if it can build from 2 path
		g_autofree GFile * source = g_file_new_build_filename(mod_folder_path, file->source, NULL);

		error_t copy_result = ERR_SUCCESS;
		if(file->isFolder) {
			g_autofree GFile * dest_folder = g_file_new_for_path(destination_folder);
			copy_result = file_recursive_copy(source, dest_folder, G_FILE_COPY_NONE, NULL, NULL);
		} else {
			GError * err = NULL;
			//see basename man page
			g_autofree char * posix_source = strdup(file->source);
			g_autofree GFile * destination = g_file_new_build_filename(destination_folder, basename(posix_source), NULL);
			if(!g_file_copy(source, destination, G_FILE_COPY_NONE, NULL, NULL, NULL, &err)) {
				fprintf(stderr, "%s\n", err->message);
				copy_result = ERR_FAILURE;
			}
		}
		if(copy_result != ERR_SUCCESS) {
			g_warning( "Copy failed, some file might be corrupted\n");
		}

		file_operation_iterator = g_list_next(file_operation_iterator);
	}

	g_list_free_full(mods, g_free);
	return ERR_SUCCESS;
}

GList * fomod_process_cond_files(const Fomod_t * fomod, GList * flag_list, GList * pending_file_operations) {
	for(int condId = 0; condId < fomod->cond_files_count; condId++) {
		const FomodCondFile_t *cond_file = &fomod->cond_files[condId];

		bool are_all_flags_valid = true;

		//checking if all flags are valid
		for(long flag_id = 0; flag_id < cond_file->flag_count; flag_id++) {
			const GList * link = g_list_find_custom(flag_list, &(cond_file->required_flags[flag_id]), (GCompareFunc)fomod_flag_equal);
			if(link == NULL) {
				are_all_flags_valid = false;
				break;
			}
		}

		if(are_all_flags_valid) {
			for(long file_id = 0; file_id < cond_file->file_count; file_id++) {
				const FomodFile_t * file = &(cond_file->files[file_id]);

				FomodFile_t * file_copy = g_malloc(sizeof(*file));
				*file_copy = *file;

				file_copy->destination = strdup(file->destination);
				file_copy->source = strdup(file->source);

				pending_file_operations = g_list_append(pending_file_operations, file_copy);
			}
		}
	}
	return pending_file_operations;
}

void fomod_free_file_operations(GList * file_operations) {
	GList * file_operations_start = file_operations;
	while(file_operations != NULL) {
		FomodFile_t * file = (FomodFile_t *)file_operations->data;
		if(file->destination != NULL)free(file->destination);
		if(file->source != NULL)free(file->source);
		file_operations = g_list_next(file_operations);
	}

	g_list_free_full(file_operations_start, free);
}

void fomod_free_fomod(Fomod_t * fomod) {
	for(int i = 0; i < fomod->cond_files_count; i++) {
		FomodCondFile_t * cond_file = &(fomod->cond_files[i]);
		for(long file_id = 0; file_id < cond_file->file_count; file_id++) {
			free(cond_file->files[file_id].destination);
			free(cond_file->files[file_id].source);
		}

		for(long flag_id = 0; flag_id < cond_file->flag_count; flag_id++) {
			FomodFlag_t * flag = &(cond_file->required_flags[flag_id]);
			free(flag->name);
			free(flag->value);
		}
		free(cond_file->files);
		free(cond_file->required_flags);
	}
	free(fomod->cond_files);
	free(fomod->module_image);
	free(fomod->module_name);

	int size = fomod_count_until_null(fomod->required_install_files);
	for(int i = 0; i < size; i++) {
		free(fomod->required_install_files[i]);
	}
	free(fomod->required_install_files);

	for(int i = 0; i < fomod->step_count; i++) {
		FomodStep_t * step = &fomod->steps[i];
		for(int group_id = 0; group_id < step->group_count; group_id++) {
			FomodGroup_t * group = &step->groups[group_id];
			grp_free_group(group);
		}
		for(int flag_id = 0; flag_id < step->flag_count; flag_id++) {
			FomodFlag_t * flag = &(step->required_flags[flag_id]);
			free(flag->name);
			free(flag->value);
		}
		free(step->groups);
		free(step->required_flags);
		free(step->name);
	}
	free(fomod->steps);

	//set every counter to zero and every pointer to null
	memset(fomod, 0, sizeof(Fomod_t));
}
