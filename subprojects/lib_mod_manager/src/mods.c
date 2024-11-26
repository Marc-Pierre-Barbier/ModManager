#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <constants.h>
#include <file.h>
#include <unistd.h>
#include "mods.h"
#include "archives.h"

//no function are declared in main it's just macros
#include <constants.h>


#include "steam.h"

typedef struct Mod {
	int modId;
	char * path;
	char * name;
} Mod_t;

static gint compare_order(const void * a, const void * b) {
	const Mod_t * mod_a = a;
	const Mod_t * mod_b = b;

	if(mod_a->modId == -1) return -1;
	if(mod_b->modId == -1) return 1;

	return mod_a->modId - mod_b->modId;
}

GList * mods_list(int appid) {
	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	const char * home = g_get_home_dir();
	g_autofree const char * mod_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, NULL);

	GList * list = NULL;
	DIR *d;
	struct dirent *dir;
	d = opendir(mod_folder);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			//removes .. & . from the list
			if(strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {
				int modId = -1;
				//TODO: remove order file
				char * mod_path = g_build_filename(mod_folder, dir->d_name, NULL);
				char * mod_order = g_build_filename(mod_path, ORDER_FILE, NULL);
				FILE * fd_order = fopen(mod_order, "r");
				g_free(mod_order);
				if(fd_order != NULL) {
					fscanf(fd_order, "%d", &modId);
					fclose(fd_order);
				}

				Mod_t * mod = alloca(sizeof(Mod_t));
				mod->modId = modId;
				//we are going to return this value so no alloca here
				mod->name = strdup(dir->d_name);
				//strdup but on the stack
				//add one for the \0
				mod->path = alloca((strlen(mod_path) + 1) * sizeof(char));
				memcpy(mod->path, mod_path, (strlen(mod_path) + 1) * sizeof(char));


				list = g_list_append(list, mod);
				g_free(mod_path);
			}
		}
		closedir(d);
	}

	list = g_list_sort(list, compare_order);
	GList * ordered_mods = NULL;

	int index = 0;
	for(GList * p_list = list; p_list != NULL; p_list = g_list_next(p_list)) {
		Mod_t * mod = p_list->data;

		gchar * mod_order = g_build_filename(mod->path, ORDER_FILE, NULL);

		FILE * fd_order = fopen(mod_order, "w+");
		g_free(mod_order);
		if(fd_order != NULL) {
			fprintf(fd_order, "%d", index);
			fclose(fd_order);
		}

		ordered_mods = g_list_append(ordered_mods, mod->name);

		index++;
	}

	//do not use g_list_free_full since i used alloca everything is on the stack
	g_list_free(list);
	return ordered_mods;
}


error_t mods_swap_place(int appid, int mod_id_a, int mod_id_b) {
	char appidStr[10];
	sprintf(appidStr, "%d", appid);

	g_message("flipping mod%d and mod%d", mod_id_a, mod_id_b);

	const char * home = g_get_home_dir();
	g_autofree char * mod_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidStr, NULL);

	GList * list = mods_list(appid);
	GList * list_a = g_list_nth(list, mod_id_a);
	GList * list_b = g_list_nth(list, mod_id_b);;

	if(list_a == NULL || list_b == NULL) {
		g_warning( "Invalid modId\n");
		return ERR_FAILURE;
	}

	g_autofree char * mod_a_folder = g_build_filename(mod_folder, list_a->data, ORDER_FILE, NULL);
	g_autofree char * mod_b_folder = g_build_filename(mod_folder, list_b->data, ORDER_FILE, NULL);

	FILE * file_a = fopen(mod_a_folder, "w");
	FILE * file_b = fopen(mod_b_folder, "w");

	if(file_a == NULL || file_b == NULL) {
		g_warning( "Error could not open order file\n");
		return ERR_FAILURE;
	}

	fprintf(file_a, "%d", mod_id_b);
	fprintf(file_b, "%d", mod_id_a);

	fclose(file_a);
	fclose(file_b);

	g_list_free_full(list, free);
	return ERR_SUCCESS;
}

mods_mod_detail_t mods_mod_details(const int appid, int modid) {
	char appIdStr[GAMES_MAX_APPID_LENGTH];
	snprintf(appIdStr, GAMES_MAX_APPID_LENGTH, "%d", appid);

	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modid);
	const char * mod_name = (char*)mod->data;

	const char * home = g_get_home_dir();
	g_autofree char * mod_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appIdStr, NULL);
	g_autofree char * current_mod_path = g_build_filename(mod_folder, mod_name, NULL);
	g_autofree char * current_mod_install_flag =  g_build_filename(current_mod_path, INSTALLED_FLAG_FILE, NULL);
	g_autofree char * current_mod_fomod_file =  g_build_filename(current_mod_path, "fomod", "moduleconfig.xml", NULL);
	g_autofree char * fomod_mod_name = g_strconcat(mod_name, "__FOMOD", NULL);
	g_autofree char * fomod_mod_folder = g_build_filename(mod_folder, fomod_mod_name, NULL);


	mods_mod_detail_t result;
	result.is_present = access(current_mod_path, F_OK) == 0;
	result.is_activated = access(current_mod_install_flag, F_OK) == 0;
	result.has_fomodfile = access(current_mod_fomod_file, F_OK) == 0;
	result.is_fomod = strstr(current_mod_path, "__FOMOD") != NULL;
	result.has_fomod_sibling = access(fomod_mod_folder, F_OK) == 0;

	g_list_free_full(mods, free);
	return result;
}


error_t mods_enable_mod(int appid, int modId) {
	int return_value = EXIT_SUCCESS;
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modId);

	char appidstr[GAMES_MAX_APPID_LENGTH];
	snprintf(appidstr, GAMES_MAX_APPID_LENGTH, "%d", appid);

	const char * home = g_get_home_dir();
	g_autofree char * mod_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);

	const char * modName = (char*)mod->data;
	g_autofree char * modFlag = g_build_filename(mod_folder, modName, INSTALLED_FLAG_FILE, NULL);

	if(access(mod_folder, F_OK) != 0) {
		//mod not found
		return_value = EXIT_FAILURE;
		goto exit;
	}

	if(access(modFlag, F_OK) == 0) {
		//mod already enabled
		return_value = EXIT_SUCCESS;
		goto exit;
	}

	//Create activated file
	FILE * fd = fopen(modFlag, "w+");
	if(fd != NULL) {
		int exit_code = (fwrite("", 1, 1, fd) != 1);
		exit_code |= fclose(fd);
		if(exit_code != 0)
			return_value = EXIT_FAILURE;
	} else {
		return_value = EXIT_FAILURE;
	}

exit:
	g_list_free_full(mods, free);
	return return_value;
}

error_t mods_disable_mod(int appid, int modId) {
	int return_value = EXIT_SUCCESS;
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modId);

	char appidstr[GAMES_MAX_APPID_LENGTH];
	snprintf(appidstr, GAMES_MAX_APPID_LENGTH, "%d", appid);

	const char * home = g_get_home_dir();
	g_autofree char * mod_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);

	const char * mod_name = (char*)mod->data;
	g_autofree char * mod_flag = g_build_filename(mod_folder, mod_name, INSTALLED_FLAG_FILE, NULL);

	if(access(mod_folder, F_OK) != 0) {
		//mod not found
		return_value = EXIT_FAILURE;
		goto exit;
	}

	if(access(mod_flag, F_OK) != 0) {
		//mod already disabled
		return_value = EXIT_SUCCESS;
		goto exit;
	}

	if(unlink(mod_flag) != 0) {
		//could not disable the mod
		return_value = EXIT_FAILURE;
		goto exit;
	}

exit:
	g_list_free_full(mods, free);
	return return_value;
}

error_t mods_remove_mod(int appid, int modId) {
	GFile * folder = mods_get_mod_folder(appid, modId);
	if(folder == NULL)
		return ERR_FAILURE;

	GError * err = NULL;
	//I gave up, i could not figure out why g_file_trash was failing if you delete a fomod you just created
	//so i wired up file_delete_recursive which at the same time improve support for fs without trash handling
	if(!g_file_trash(folder, NULL, &err)) {
		printf("%s\n", err->message);
		g_clear_error(&err);
		if(!file_delete_recursive(folder, NULL, &err)) {
			printf("%s\n", err->message);
			g_clear_error(&err);
			return ERR_FAILURE;
		} else {
			printf("It's safe to ignore the previous error \n");
		}
	}
	return ERR_SUCCESS;
}

GFile * mods_get_mods_folder(int appid) {
	char appidstr[GAMES_MAX_APPID_LENGTH];
	snprintf(appidstr, GAMES_MAX_APPID_LENGTH, "%d", appid);
	const char * home = g_get_home_dir();
	return g_file_new_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);
}

GFile * mods_get_mod_folder(int appid, int mod_id) {
	g_autofree GFile * mods_folder = mods_get_mods_folder(appid);
	char * mods_folder_path = g_file_get_path(mods_folder);
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, mod_id);

	GFile * mod_folder = NULL;

	if(mod == NULL) {
		g_warning( "Mod not found\n");
		goto exit;
	}

	mod_folder = g_file_new_build_filename(mods_folder_path, mods->data, NULL);
	g_list_free_full(mods, free);
exit:
	return mod_folder;
}

static bool search_fomod_data(GFile * path_file, int appid) {
	g_autofree const char * path = g_file_get_path(path_file);
	g_autofree GFile * fomod = g_file_new_build_filename(path, "fomod", NULL);

	if(g_file_query_exists(fomod, NULL)) {
		return true;
	}

	const int gameId = steam_game_id_from_app_id(appid);
	g_autofree char * target = g_ascii_strdown(GAMES_MOD_TARGET[gameId], -1);
	g_autofree GFile * data = g_file_new_build_filename(path, target, NULL);
	return g_file_query_exists(data, NULL);
}

static bool check_for_unwrap(GFile * path_file, int appid) {
	/* Some mods come with a single folder inside their archive these mods need to be unwraped */
	g_autofree char * path = g_file_get_path(path_file);
	const int gameId = steam_game_id_from_app_id(appid);
	g_autofree char * target = g_ascii_strdown(GAMES_MOD_TARGET[gameId], -1);

	const gchar *filename;
	GDir * dir = g_dir_open(path, 0, NULL);
	int filecount = 0;
	while ((filename = g_dir_read_name(dir))) {
		if(strcmp(filename, "fomod") == 0 || strcmp(filename, target) == 0) {
			filecount = INT_MAX;
			break;
		}
		filecount++;
		if(filecount > 1)
			break;
	}

	//equal to 1 mean unwrap is needed
	return filecount == 1;
}

error_t mods_add_mod(GFile * file_path, int app_id) {
	g_autofree GFileInfo * file_info = g_file_query_info(file_path, "access::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

	gboolean can_read = g_file_info_get_attribute_boolean (file_info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
	gboolean exits = g_file_query_exists(file_path, NULL);

	if (!exits) {
		g_warning("File not found\n");
		return ERR_FAILURE;
	}

	if (!can_read) {
		g_warning("File not readable\n");
		return ERR_FAILURE;
	}

	g_autofree char * config_folder = g_build_filename(getenv("HOME"), MODLIB_WORKING_DIR, NULL);
	if(g_mkdir_with_parents(config_folder, 0755) < 0) {
		g_warning( "Could not create mod folder\n");
		return ERR_FAILURE;
	}

	const char * archive_name = g_file_get_basename(file_path);
	const char * extension = file_extract_extension(archive_name);
	g_autofree char * lowercase_extension = g_ascii_strdown(extension, -1);

	const int length_no_extension = strlen(archive_name) - strlen(extension); // we replace the . by \0 so ne need for +1
	char filename[length_no_extension];
	memcpy(filename, archive_name, length_no_extension);
	filename[length_no_extension - 1] = '\0';

	char appIdStr[20];
	sprintf(appIdStr, "%d", app_id);
	g_autofree GFile * outdir = g_file_new_build_filename(config_folder, MOD_FOLDER_NAME, appIdStr, filename, NULL);

	if(!g_file_make_directory_with_parents(outdir, NULL, NULL)) {
		//TODO: memory management
		return EXIT_FAILURE;
	}

	printf("Adding mod, this process can be slow depending on your hardware\n");
	int return_value = archive_deflate(file_path, outdir);
	if(return_value != AR_ERR_OK) { //TODO: add more errors
		g_warning( "Error occured during decompression are supported\n");
		return_value = EXIT_FAILURE;
	}

	#define EXIT_FAIL(a) if(!a) { return_value = EXIT_FAILURE; goto exit; }

	g_autofree GFile * tmp_dir = g_file_new_build_filename(config_folder, MOD_FOLDER_NAME, appIdStr, "TMP", NULL);
	g_autofree char * outdir_str = g_file_get_path(outdir);

	if(check_for_unwrap(outdir, app_id)) {
		GDir * dir = g_dir_open(outdir_str, 0, NULL);
		const char * filename = g_dir_read_name(dir);
		if(filename != NULL) {
			EXIT_FAIL(g_file_move(outdir, tmp_dir, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL))
			g_autofree GFile * wrapper_file = g_file_new_build_filename(config_folder, MOD_FOLDER_NAME, appIdStr, "TMP", filename, NULL);
			EXIT_FAIL(g_file_move(wrapper_file, outdir, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL))
		}
	}


	if(!search_fomod_data(outdir, app_id)) {
		const int gameId = steam_game_id_from_app_id(app_id);

		if(g_file_query_exists(tmp_dir, NULL)) {
			EXIT_FAIL(g_file_delete(tmp_dir, NULL, NULL))
		}

		EXIT_FAIL(g_file_move(outdir, tmp_dir, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL))
		char * target = g_ascii_strdown(GAMES_MOD_TARGET[gameId], -1);
		g_autofree GFile * outdir2 = g_file_new_build_filename(outdir_str, target, NULL);
		g_mkdir_with_parents(outdir_str, 0777);
		EXIT_FAIL(g_file_move(tmp_dir, outdir2, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL))
	}

exit:
	if(g_file_query_exists(tmp_dir, NULL)) {
		if(!g_file_delete(tmp_dir, NULL, NULL)) {
			g_warning("Could not delete tmp file");
		}
	}
	if(return_value == EXIT_FAILURE) {
		g_warning("Failed to install mod\n");
		g_file_trash(outdir, NULL, NULL);
		return ERR_FAILURE;
	} else {
		g_info("Done\n");
	}

	return ERR_SUCCESS;
}
