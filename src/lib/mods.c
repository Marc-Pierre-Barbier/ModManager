#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <constants.h>
#include <getHome.h>
#include <file.h>
#include <unistd.h>
#include "mods.h"
#include "archives.h"

//no function are declared in main it's just macros
#include <constants.h>

#include "getHome.h"

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
	char appid_str[10];
	sprintf(appid_str, "%d", appid);

	g_autofree GFile * home_file = audit_get_home();
	g_autofree char * home = g_file_get_path(home_file);
	g_autofree char * mod_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, NULL);

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

	g_autofree GFile * home_file = audit_get_home();
	g_autofree char * home = g_file_get_path(home_file);
	g_autofree char * mod_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidStr, NULL);

	GList * list = mods_list(appid);
	GList * list_a = g_list_nth(list, mod_id_a);
	GList * list_b = g_list_nth(list, mod_id_b);;

	if(list_a == NULL || list_b == NULL) {
		g_error( "Invalid modId\n");
		return ERR_FAILURE;
	}

	g_autofree char * mod_a_folder = g_build_filename(mod_folder, list_a->data, ORDER_FILE, NULL);
	g_autofree char * mod_b_folder = g_build_filename(mod_folder, list_b->data, ORDER_FILE, NULL);

	FILE * file_a = fopen(mod_a_folder, "w");
	FILE * file_b = fopen(mod_b_folder, "w");

	if(file_a == NULL || file_b == NULL) {
		g_error( "Error could not open order file\n");
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
	char appIdStr[10];
	snprintf(appIdStr, 10, "%d", appid);

	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modid);
	const char * mod_name = (char*)mod->data;

	g_autofree GFile * home_file = audit_get_home();
	g_autofree char * home = g_file_get_path(home_file);
	g_autofree char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appIdStr, NULL);
	g_autofree char * currentModPath = g_build_filename(modFolder, mod_name, NULL);
	g_autofree char * currentModInstallFlag =  g_build_filename(currentModPath, INSTALLED_FLAG_FILE, NULL);
	g_autofree char * currentModFomodFile =  g_build_filename(currentModPath, "fomod", "moduleconfig.xml", NULL);
	g_autofree char * fomodModName = g_strconcat(mod_name, "__FOMOD", NULL);
	g_autofree char * fomodModFolder = g_build_filename(modFolder, fomodModName, NULL);


	mods_mod_detail_t result;
	result.is_present = access(currentModPath, F_OK) == 0;
	result.is_activated = access(currentModInstallFlag, F_OK) == 0;
	result.has_fomodfile = access(currentModFomodFile, F_OK) == 0;
	result.is_fomod = strstr(currentModPath, "__FOMOD") != NULL;
	result.has_fomod_sibling = access(fomodModFolder, F_OK) == 0;

	g_list_free_full(mods, free);
	return result;
}


error_t mods_enable_mod(int appid, int modId) {
	int returnValue = EXIT_SUCCESS;
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modId);

	char appidstr[9];
	snprintf(appidstr, 9, "%d", appid);

	g_autofree GFile * home_file = audit_get_home();
	g_autofree char * home = g_file_get_path(home_file);
	g_autofree char * mod_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);

	const char * modName = (char*)mod->data;
	g_autofree char * modFlag = g_build_filename(mod_folder, modName, INSTALLED_FLAG_FILE, NULL);

	if(access(mod_folder, F_OK) != 0) {
		//mod not found
		returnValue = EXIT_FAILURE;
		goto exit;
	}

	if(access(modFlag, F_OK) == 0) {
		//mod already enabled
		returnValue = EXIT_SUCCESS;
		goto exit;
	}

	//Create activated file
	FILE * fd = fopen(modFlag, "w+");
	if(fd != NULL) {
		int exit_code = (fwrite("", 1, 1, fd) != 1);
		exit_code |= fclose(fd);
		if(exit_code != 0)
			returnValue = EXIT_FAILURE;
	} else {
		returnValue = EXIT_FAILURE;
	}

exit:
	g_list_free_full(mods, free);
	return returnValue;
}

error_t mods_disable_mod(int appid, int modId) {
	int returnValue = EXIT_SUCCESS;
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modId);

	char appidstr[9];
	snprintf(appidstr, 9, "%d", appid);

	g_autofree GFile * home_file = audit_get_home();
	g_autofree char * home = g_file_get_path(home_file);
	g_autofree char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);

	const char * modName = (char*)mod->data;
	g_autofree char * modFlag = g_build_filename(modFolder, modName, INSTALLED_FLAG_FILE, NULL);

	if(access(modFolder, F_OK) != 0) {
		//mod not found
		returnValue = EXIT_FAILURE;
		goto exit;
	}

	if(access(modFlag, F_OK) != 0) {
		//mod already disabled
		returnValue = EXIT_SUCCESS;
		goto exit;
	}

	if(unlink(modFlag) != 0) {
		//could not disable the mod
		returnValue = EXIT_FAILURE;
		goto exit;
	}

exit:
	g_list_free_full(mods, free);
	return returnValue;
}

error_t mods_remove_mod(int appid, int modId) {
	GFile * folder = mods_get_mod_folder(appid, modId);
	if(folder == NULL)
		return ERR_FAILURE;

	if(!g_file_trash(folder, NULL, NULL))
		return ERR_FAILURE;
	return ERR_SUCCESS;
}

GFile * mods_get_mods_folder(int appid) {
	char appidstr[9];
	snprintf(appidstr, 9, "%d", appid);
	g_autofree GFile * home_file = audit_get_home();
	g_autofree char * home = g_file_get_path(home_file);
	return g_file_new_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);
}

GFile * mods_get_mod_folder(int appid, int mod_id) {
	g_autofree GFile * mods_folder = mods_get_mods_folder(appid);
	char * mods_folder_path = g_file_get_path(mods_folder);
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, mod_id);

	GFile * mod_folder = NULL;

	if(mod == NULL) {
		g_error( "Mod not found\n");
		goto exit;
	}

	mod_folder = g_file_new_build_filename(mods_folder_path, mods->data, NULL);
	g_list_free_full(mods, free);
exit:
	return mod_folder;
}

//TODO: Replace print with errors
error_t mods_add_mod(GFile * file_path, int appId) {
	g_autofree GFileInfo * file_info = g_file_query_info(file_path, "access::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

	gboolean can_read = g_file_info_get_attribute_boolean (file_info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
	gboolean exits = g_file_query_exists(file_path, NULL);

	if (!exits) {
		g_error("File not found\n");
		return ERR_FAILURE;
	}

	if (!can_read) {
		g_error("File not readable\n");
		return ERR_FAILURE;
	}

	g_autofree char * configFolder = g_build_filename(getenv("HOME"), MODLIB_WORKING_DIR, NULL);
	if(g_mkdir_with_parents(configFolder, 0755) < 0) {
		g_error( "Could not create mod folder\n");
		return ERR_FAILURE;
	}

	const char * filename = g_file_get_basename(file_path);
	const char * extension = file_extract_extension(filename);
	g_autofree char * lowercaseExtension = g_ascii_strdown(extension, -1);

	char appIdStr[20];
	sprintf(appIdStr, "%d", appId);
	g_autofree GFile * outdir = g_file_new_build_filename(configFolder, MOD_FOLDER_NAME, appIdStr, filename, NULL);

	if(!g_file_make_directory_with_parents(outdir, NULL, NULL)) {
		//TODO: memory management
		return EXIT_FAILURE;
	}

	int returnValue = EXIT_SUCCESS;
	printf("Adding mod, this process can be slow depending on your hardware\n");
	if(strcmp(lowercaseExtension, "rar") == 0) {
		returnValue = archive_unrar(file_path, outdir);
	} else if (strcmp(lowercaseExtension, "zip") == 0) {
		returnValue = archive_unzip(file_path, outdir);
	} else if (strcmp(lowercaseExtension, "7z") == 0) {
		returnValue = archive_un7z(file_path, outdir);
	} else {
		g_error( "Unsupported format only zip/7z/rar are supported\n");
		returnValue = EXIT_FAILURE;
	}

	if(returnValue == EXIT_FAILURE) {
		printf("Failed to install mod\n");
		g_file_trash(outdir, NULL, NULL);
		return ERR_FAILURE;
	}
	else
		printf("Done\n");

	return ERR_SUCCESS;
}
