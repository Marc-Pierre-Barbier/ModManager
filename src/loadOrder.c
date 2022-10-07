#include "loadOrder.h"
#include "main.h"
#include "steam.h"
#include "file.h"

#include <stdio.h>
#include <unistd.h>

error_t listPlugins(int appid, GList ** plugins) {
	GHashTable * gamePaths;
	error_t status = search_games(&gamePaths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	int gameId = getGameIdFromAppId(appid);
	if(gameId < 0 ) {
		return ERR_FAILURE;
	}

	//save appid parsing
	//TODO: apply a similar mechanism everywhere
	size_t appidStrLen = snprintf(NULL, 0, "%d", appid) + 1;
	char appidStr[appidStrLen];
	sprintf(appidStr, "%d", appid);



	const char * path = g_hash_table_lookup(gamePaths, &appid);
	char * steamGameFolder = g_build_filename(path, "steamapps/common", GAMES_NAMES[gameId], "Data", NULL);

	//esp && esm files are loadable
	DIR * d = opendir(steamGameFolder);
	struct dirent *dir;
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			const char * extension = extractExtension(dir->d_name);
			if(strcmp(extension, "esp") == 0 || strcmp(extension, "esm") == 0) {
				*plugins = g_list_append(*plugins, strdup(dir->d_name));
			}
		}
	}

	g_free(steamGameFolder);
	return ERR_SUCCESS;
}

error_t getLoadOrder(int appid, GList ** order) {

	GHashTable * gamePaths;
	error_t status = search_games(&gamePaths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	GList * l_plugins = NULL;
	error_t error = listPlugins(appid, &l_plugins);
	if(error == ERR_FAILURE)
		return ERR_FAILURE;


	int gameId = getGameIdFromAppId(appid);
	if(gameId < 0 ) {
		return ERR_FAILURE;
	}

	size_t appidStrLen = snprintf(NULL, 0, "%d", appid) + 1;
	char appidStr[appidStrLen];
	sprintf(appidStr, "%d", appid);

	const char * path = g_hash_table_lookup(gamePaths, &appid);
	char * loadOrderPath = g_build_filename(path, "steamapps/compatdata", appidStr, "pfx/drive_c/users/steamuser/AppData/Local/", GAMES_NAMES[gameId], "loadorder.txt", NULL);

	GList * l_currentLoadOrder = NULL;
	if(access(loadOrderPath, F_OK)) {
		FILE * f_loadOrder = fopen(loadOrderPath, "r");

		size_t length = 0;
		char * line = NULL;

		while(getline(&line, &length, f_loadOrder) > 0) {
			if(line[0] == '#' || line[0] == '\n')
				continue;

			l_currentLoadOrder = g_list_append(l_currentLoadOrder, strdup(line));
		}
	}


	GList * l_currentLoadOrderCursor = l_currentLoadOrder;
	GList * l_completeLoadOrder = NULL;

	while(l_currentLoadOrderCursor != NULL) {
		char * modName = l_currentLoadOrderCursor->data;
		//TODO: finir sa
		GList * mod = g_list_find_custom(l_plugins, modName, (GCompareFunc)strcmp);
		if(mod == NULL) {
			//The plugin is no longer installed
			continue;
		} else {
			l_completeLoadOrder = g_list_append(l_completeLoadOrder, strdup(modName));
		}
		l_currentLoadOrderCursor = g_list_next(l_currentLoadOrderCursor);
	}

	*order = l_completeLoadOrder;

	g_list_free_full(l_plugins, free);
	g_list_free_full(l_currentLoadOrder, free);
	g_free(loadOrderPath);
	return ERR_SUCCESS;
}

error_t setLoadOrder(int appid, GList * loadOrder) {
	GHashTable * gamePaths;
	error_t status = search_games(&gamePaths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	int gameId = getGameIdFromAppId(appid);
	if(gameId < 0 ) {
		return ERR_FAILURE;
	}

	size_t appidStrLen = snprintf(NULL, 0, "%d", appid) + 1;
	char appidStr[appidStrLen];
	sprintf(appidStr, "%d", appid);

	const char * path = g_hash_table_lookup(gamePaths, &appid);
	char * loadOrderPath = g_build_filename(path, "steamapps/compatdata", appidStr, "pfx/drive_c/users/steamuser/AppData/Local/", GAMES_NAMES[gameId], "loadorder.txt", NULL);

	FILE * f_loadOrder = fopen(loadOrderPath, "w");
	while(loadOrder != NULL) {
		fwrite(loadOrder->data, sizeof(char), strlen(loadOrder->data), f_loadOrder);
		loadOrder = g_list_next(loadOrder);
	}
	fclose(f_loadOrder);

	return ERR_SUCCESS;
}
