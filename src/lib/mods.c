#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <constants.h>
#include <getHome.h>
#include <install.h>
#include <file.h>
#include <unistd.h>
#include "mods.h"

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

	char * home = getHome();
	char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidStr, NULL);
	free(home);

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
	g_free(modFolder);
	return orderedMods;
}


error_t mods_swap_place(int appid, int modIdA, int modIdB) {
	char appidStr[10];
	sprintf(appidStr, "%d", appid);

	char * home = getHome();
	char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidStr, NULL);
	free(home);

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
		fprintf(stderr, "Invalid modId\n");
		return ERR_FAILURE;
	}

	char * modAFolder = g_build_filename(modFolder, listA->data, ORDER_FILE, NULL);
	char * modBFolder = g_build_filename(modFolder, listB->data, ORDER_FILE, NULL);
	g_free(modFolder);

	FILE * fileA = fopen(modAFolder, "w");
	FILE * fileB = fopen(modBFolder, "w");

	g_free(modAFolder);
	g_free(modBFolder);

	if(fileA == NULL || fileB == NULL) {
		fprintf(stderr, "Error could not open order file\n");
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

	char * home = getHome();
	char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appIdStr, NULL);
	char * currentModPath = g_build_filename(modFolder, mod_name, NULL);
	char * currentModInstallFlag =  g_build_filename(currentModPath, INSTALLED_FLAG_FILE, NULL);
	char * currentModFomodFile =  g_build_filename(currentModPath, "fomod", "moduleconfig.xml", NULL);
	char * fomodModName = g_strconcat(mod_name, "__FOMOD", NULL);
	char * fomodModFolder = g_build_filename(modFolder, fomodModName, NULL);


	mods_mod_detail_t result;
	result.is_present = access(currentModPath, F_OK) == 0;
	result.is_activated = access(currentModInstallFlag, F_OK) == 0;
	result.has_fomodfile = access(currentModFomodFile, F_OK) == 0;
	result.is_fomod = strstr(currentModPath, "__FOMOD") == 0;
	result.has_fomod_sibling = access(fomodModFolder, F_OK) == 0;


	g_free(currentModFomodFile);
	g_free(currentModInstallFlag);
	g_free(currentModPath);
	g_free(fomodModName);
	g_free(fomodModFolder);
	g_list_free_full(mods, free);
	return result;
}


error_t mods_enable_mod(int appid, int modId) {
	int returnValue = EXIT_SUCCESS;
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modId);

	char appidstr[9];
	snprintf(appidstr, 9, "%d", appid);

	char * home = getHome();
	char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);
	free(home);
	const char * modName = (char*)mod->data;
	char * modFlag = g_build_filename(modFolder, modName, INSTALLED_FLAG_FILE, NULL);

	if(access(modFolder, F_OK) != 0) {
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
	g_free(modFolder);
	g_list_free_full(mods, free);
	g_free(modFlag);
	return returnValue;
}

error_t mods_disable_mod(int appid, int modId) {
	int returnValue = EXIT_SUCCESS;
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, modId);

	char appidstr[9];
	snprintf(appidstr, 9, "%d", appid);

	char * home = getHome();
	char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);
	free(home);
	const char * modName = (char*)mod->data;
	char * modFlag = g_build_filename(modFolder, modName, INSTALLED_FLAG_FILE, NULL);

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
	g_free(modFolder);
	g_list_free_full(mods, free);
	g_free(modFlag);
	return returnValue;
}

error_t mods_remove_mod(int appid, int modId) {
	char * folder = mods_get_mod_folder(appid, modId);
	if(folder == NULL)
		return ERR_FAILURE;

	if(file_delete(folder, true))
		return ERR_FAILURE;
	return ERR_SUCCESS;
}

char * mods_get_mods_folder(int appid) {
	char appidstr[9];
	snprintf(appidstr, 9, "%d", appid);
	char * home = getHome();
	char * mods_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appidstr, NULL);
	free(home);
	return mods_folder;
}

char * mods_get_mod_folder(int appid, int mod_id) {
	char * mods_folder = mods_get_mods_folder(appid);
	GList * mods = mods_list(appid);
	GList * mod = g_list_nth(mods, mod_id);

	gchar * mod_folder = NULL;

	if(mod == NULL) {
		fprintf(stderr, "Mod not found\n");
		goto exit;
	}

	mod_folder = g_build_filename(mods_folder, mods->data, NULL);
	g_free(mods_folder);
	g_list_free_full(mods, free);

exit:
	return mod_folder;
}
