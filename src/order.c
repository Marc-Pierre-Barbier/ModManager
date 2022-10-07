#include <stdio.h>
#include <stdlib.h>
#include "order.h"

//no function are declared in main it's just macros
#include "main.h"
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

GList * listMods(int appid) {
	char appidStr[10];
	sprintf(appidStr, "%d", appid);

	char * home = getHome();
	char * modFolder = g_build_filename(home, MANAGER_FILES, MOD_FOLDER_NAME, appidStr, NULL);
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

	//do not use g_list_free since i used alloca everything is on the stack
	g_list_free(list);
	g_free(modFolder);
	return orderedMods;
}


error_t swapPlace(int appid, int modIdA, int modIdB) {
	char appidStr[10];
	sprintf(appidStr, "%d", appid);

	char * home = getHome();
	char * modFolder = g_build_filename(home, MANAGER_FILES, MOD_FOLDER_NAME, appidStr, NULL);
	free(home);

	GList * list = listMods(appid);
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
