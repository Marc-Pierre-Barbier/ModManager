#include "loadOrder.h"
#include "main.h"
#include "steam.h"
#include "file.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "getDataPath.h"

//TODO: detect if the game is running
//TODO: deploy the game

error_t listPlugins(int appid, GList ** plugins) {


	//save appid parsing
	//TODO: apply a similar mechanism everywhere
	size_t appidStrLen = snprintf(NULL, 0, "%d", appid) + 1;
	char appidStr[appidStrLen];
	sprintf(appidStr, "%d", appid);

	char * dataFolder = NULL;
	error_t error = getDataPath(appid, &dataFolder);
	if(error != ERR_SUCCESS) {
		return ERR_FAILURE;
	}

	//esp && esm files are loadable
	DIR * d = opendir(dataFolder);
	struct dirent *dir;
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			const char * extension = extractExtension(dir->d_name);
			if(strcmp(extension, "esp") == 0 || strcmp(extension, "esm") == 0) {
				*plugins = g_list_append(*plugins, strdup(dir->d_name));
			}
		}
	}

	free(dataFolder);
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

//TODO: support compression since it can change how we read the file
//https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format#Records
//https://www.mwmythicmods.com/argent/tech/es_format.html
error_t getModDependencies(const char * esmPath, GList ** dependencies) {
	FILE * file = fopen(esmPath, "r");

	char sectionName[5];
	sectionName[4] = '\0';
	fread(sectionName, sizeof(char), 4, file);

	size_t sizeFieldSize = 0;
	u_int8_t recordHeaderToIgnore = 0;

	//the field "length" in the sub-record change between games.
	//TES4 => Fallout4 Oblivion Skyrim(+SE)
	//TES3 => Morrowind
	if(strcmp(sectionName, "TES3") == 0) {
		printf("Using tes3 file format\n");
		recordHeaderToIgnore = 8;
		sizeFieldSize = 4;
	} else if(strcmp(sectionName, "TES4") == 0) {
		printf("Using tes4 file format\n");
		recordHeaderToIgnore = 16;
		sizeFieldSize = 2;
	} else {
		fprintf(stderr, "Unrecognized file format %s\n", sectionName);
		fclose(file);
		return ERR_FAILURE;
	}


	u_int32_t lengthVal = 0;
	fread(&lengthVal, 4, 1, file);
	//ignore the rest of the data
	fseek(file, recordHeaderToIgnore, SEEK_CUR);

	int64_t length = lengthVal;
	while(length > 0) {
		char sectionName[5];
		sectionName[4] = '\0';
		fread(sectionName, sizeof(char), 4, file);
		unsigned long subsectionLength = 0;
		fread(&subsectionLength, sizeFieldSize, 1, file);

		length -= 8 + subsectionLength;
		if(strcmp(sectionName, "MAST") == 0) {
			char * dependency = malloc(subsectionLength + 1);
			dependency[subsectionLength] = '\0';
			fread(dependency, sizeof(char), subsectionLength, file);
			*dependencies = g_list_append(*dependencies, dependency);
		} else {
			fseek(file, subsectionLength, SEEK_CUR);
		}
	}
	fclose(file);
	return ERR_SUCCESS;
}
