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

static gint compareOrder(const void * a, const void * b) {
	const Mod_t * ModA = a;
	const Mod_t * ModB = b;

	if(ModA->modId == -1) return -1;
	if(ModB->modId == -1) return 1;

	return ModA->modId - ModB->modId;
}

GList * mods_list(int appid) {
	char appidStr[10];
	sprintf(appidStr, "%d", appid);

	g_autofree GFile * home_file = audit_get_home();
	g_autofree char * home = g_file_get_path(home_file);
	g_autofree char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidStr, NULL);

	GList * list = NULL;
	DIR *d;
	struct dirent *dir;
	d = opendir(modFolder);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			//removes .. & . from the list
			if(strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {
				int modId = -1;
				//TODO: remove order file
				char * modPath = g_build_filename(modFolder, dir->d_name, NULL);
				char * modOrder = g_build_filename(modPath, ORDER_FILE, NULL);
				FILE * fd_oder = fopen(modOrder, "r");
				g_free(modOrder);
				if(fd_oder != NULL) {
					fscanf(fd_oder, "%d", &modId);
					fclose(fd_oder);
				}

				Mod_t * mod = alloca(sizeof(Mod_t));
				mod->modId = modId;
				//we are going to return this value so no alloca here
				mod->name = strdup(dir->d_name);
				//strdup but on the stack
				//add one for the \0
				mod->path = alloca((strlen(modPath) + 1) * sizeof(char));
				memcpy(mod->path, modPath, (strlen(modPath) + 1) * sizeof(char));


				list = g_list_append(list, mod);
				g_free(modPath);
			}
		}
		closedir(d);
	}

	list = g_list_sort(list, compareOrder);
	GList * orderedMods = NULL;

	int index = 0;
	for(GList * p_list = list; p_list != NULL; p_list = g_list_next(p_list)) {
		Mod_t * mod = p_list->data;

		gchar * modOrder = g_build_filename(mod->path, ORDER_FILE, NULL);

		FILE * fd_oder = fopen(modOrder, "w+");
		g_free(modOrder);
		if(fd_oder != NULL) {
			fprintf(fd_oder, "%d", index);
			fclose(fd_oder);
		}

		orderedMods = g_list_append(orderedMods, mod->name);

		index++;
	}

	//do not use g_list_free_full since i used alloca everything is on the stack
	g_list_free(list);
	return orderedMods;
}


error_t mods_swap_place(int appid, int modIdA, int modIdB) {
	char appidStr[10];
	sprintf(appidStr, "%d", appid);

	g_autofree GFile * home_file = audit_get_home();
	g_autofree char * home = g_file_get_path(home_file);
	g_autofree char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidStr, NULL);

	GList * list = mods_list(appid);
	GList * listA = list;
	GList * listB = list;

	for(int i = 0; i < modIdA; i++) {
		listA = g_list_next(listA);
	}

	for(int i = 0; i < modIdB; i++) {
		listB = g_list_next(listB);
	}

	if(listA == NULL || listB == NULL) {
		g_error( "Invalid modId\n");
		return ERR_FAILURE;
	}

	g_autofree char * modAFolder = g_build_filename(modFolder, listA->data, ORDER_FILE, NULL);
	g_autofree char * modBFolder = g_build_filename(modFolder, listB->data, ORDER_FILE, NULL);

	FILE * fileA = fopen(modAFolder, "w");
	FILE * fileB = fopen(modBFolder, "w");

	if(fileA == NULL || fileB == NULL) {
		g_error( "Error could not open order file\n");
		return ERR_FAILURE;
	}

	fprintf(fileA, "%d", modIdB);
	fprintf(fileB, "%d", modIdA);

	fclose(fileA);
	fclose(fileB);

	g_list_free_full(list, free);
	return ERR_SUCCESS;
}

mods_mod_detail_t mods_mod_details(const int appid, int modId) {
	char appIdStr[10];
	snprintf(appIdStr, 10, "%d", appid);

	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modId);
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
	error_t resultError = ERR_SUCCESS;

	GFileInfo * file_info = g_file_query_info(file_path, "access::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

	gboolean can_read = g_file_info_get_attribute_boolean (file_info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
	gboolean exits = g_file_query_exists(file_path, NULL);

	if (!exits) {
		g_error("File not found\n");
		resultError = ERR_FAILURE;
		goto exit;
	}

	if (!can_read) {
		g_error("File not readable\n");
		resultError = ERR_FAILURE;
		goto exit;
	}

	char * configFolder = g_build_filename(getenv("HOME"), MODLIB_WORKING_DIR, NULL);
	if(g_mkdir_with_parents(configFolder, 0755) < 0) {
		g_error( "Could not create mod folder\n");
		resultError = ERR_FAILURE;
		goto exit2;
	}

	const char * filename = g_file_get_basename(file_path);
	const char * extension = file_extract_extension(filename);
	char * lowercaseExtension = g_ascii_strdown(extension, -1);

	char appIdStr[20];
	sprintf(appIdStr, "%d", appId);
	GFile * outdir = g_file_new_build_filename(configFolder, MOD_FOLDER_NAME, appIdStr, filename, NULL);

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
		resultError = ERR_FAILURE;
	}
	else
		printf("Done\n");

	g_free(lowercaseExtension);
	g_free(outdir);
exit2:
	g_free(configFolder);
exit:
	g_free(file_info);
	return resultError;
}
