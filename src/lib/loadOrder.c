#include <constants.h>

#include "loadOrder.h"
#include "steam.h"
#include "file.h"

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "gameData.h"

static gint order_find_file_with_name(gconstpointer list_entry, gconstpointer user_data) {
	const order_plugin_entry_t * entry = (const order_plugin_entry_t *)list_entry;
	const char * filename = (const char *)user_data;
	return strcmp(entry->filename, (const char *)filename);
}

//TODO: detect if the game is running
//TODO: deploy the game or list undeployed plugins
error_t order_listPlugins(int appid, GList ** plugins) {
	//save appid parsing
	//TODO: apply a similar mechanism everywhere
	size_t appidStrLen = snprintf(NULL, 0, "%d", appid) + 1;
	char appidStr[appidStrLen];
	sprintf(appidStr, "%d", appid);

	GFile * data_folder_file = NULL;
	error_t error = gameData_get_data_path(appid, &data_folder_file);
	if(error != ERR_SUCCESS) {
		return ERR_FAILURE;
	}

	g_autofree char * data_folder = g_file_get_path(data_folder_file);

	//esp && esm files are loadable
	DIR * d = opendir(data_folder);
	struct dirent *dir;
	if (d != NULL) {
		while ((dir = readdir(d)) != NULL) {
			const char * extension = file_extract_extension(dir->d_name);
			//folder don't have file extensions
			if(extension == NULL)
				continue;

			if(strcmp(extension, "esp") == 0 || strcmp(extension, "esm") == 0) {
				*plugins = g_list_append(*plugins, strdup(dir->d_name));
			}
		}
		closedir(d);
	}

	free(data_folder_file);
	return ERR_SUCCESS;
}

static void removeCRLF_CR_LF(char * string) {
	int size = strlen(string);
	if(string[size - 1] == '\r') size--;
	if(string[size - 1] == '\n') size--;
	if(string[size - 1] == '\r') size--;
	string[size] = '\0';
}

error_t order_getLoadOrder(int appid, GList ** order) {

	GHashTable * gamePaths;
	error_t status = steam_searchGames(&gamePaths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	GList * l_plugins = NULL;
	error_t error = order_listPlugins(appid, &l_plugins);
	if(error == ERR_FAILURE)
		return ERR_FAILURE;


	int gameId = steam_gameIdFromAppId(appid);
	if(gameId < 0 ) {
		return ERR_FAILURE;
	}

	size_t appidStrLen = snprintf(NULL, 0, "%d", appid) + 1;
	char appidStr[appidStrLen];
	sprintf(appidStr, "%d", appid);

	const char * path = g_hash_table_lookup(gamePaths, &gameId);


	//this is the path i would use in windows but it seems it is not avaliable in all wine versions.
	//char * loadOrderPath = g_build_filename(path, "steamapps/compatdata", appidStr, "pfx/drive_c/users/steamuser/AppData/Local/", GAMES_NAMES[gameId], "loadorder.txt", NULL);
	char * loadOrderPath = g_build_filename(path, "steamapps/compatdata", appidStr, "pfx/drive_c/users/steamuser/Local Settings/Application Data/", GAMES_NAMES[gameId], "Plugins.txt", NULL);

	GList * l_currentLoadOrder = NULL;
	if(access(loadOrderPath, R_OK) == 0) {
		FILE * f_loadOrder = fopen(loadOrderPath, "r");

		size_t length = 0;
		char * line = NULL;

		while(getline(&line, &length, f_loadOrder) > 0) {
			if(line[0] == '#' || line[0] == '\n')
				continue;

			removeCRLF_CR_LF(line);

			l_currentLoadOrder = g_list_append(l_currentLoadOrder, strdup(line));
		}
		if(line != NULL)free(line);
	}


	GList * l_currentLoadOrderIterator = l_currentLoadOrder;
	GList * l_completeLoadOrder = NULL;

	while(l_currentLoadOrderIterator != NULL) {
		//pointer arithmetic to skip the *
		const char * modName = (const char *)l_currentLoadOrderIterator->data + 1;

		GList * mod = g_list_find_custom(l_plugins, modName, (GCompareFunc)strcmp);
		if(mod != NULL) {
			order_plugin_entry_t * new_entry = g_malloc(sizeof(order_plugin_entry_t));
			new_entry->filename = strdup(modName);
			new_entry->activated = TRUE;
			l_completeLoadOrder = g_list_append(l_completeLoadOrder, new_entry);
		}
		l_currentLoadOrderIterator = g_list_next(l_currentLoadOrderIterator);
	}

	GList * l_plugins_iterator = l_plugins;
	while(l_plugins_iterator != NULL) {
		const char * filename = (const char *)l_plugins_iterator->data;

		GList * mod = g_list_find_custom(l_completeLoadOrder, filename, order_find_file_with_name);
		if(mod == NULL) {
			order_plugin_entry_t * new_entry = g_malloc(sizeof(order_plugin_entry_t));
			new_entry->filename = strdup(filename);
			new_entry->activated = FALSE;
			l_completeLoadOrder = g_list_append(l_completeLoadOrder, new_entry);
		}

		l_plugins_iterator = g_list_next(l_plugins_iterator);
	}

	*order = l_completeLoadOrder;

	g_list_free_full(l_plugins, free);
	g_list_free_full(l_currentLoadOrder, free);
	g_free(loadOrderPath);
	return ERR_SUCCESS;
}

//TODO: Check if the default LF format is compatible with windows' CRLF in skyrim
//TODO: add loadorder.txt along with plugins.txt
error_t order_set_load_order(int appid, GList * loadOrder) {
	GHashTable * gamePaths;
	error_t status = steam_searchGames(&gamePaths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	int gameId = steam_gameIdFromAppId(appid);
	if(gameId < 0 ) {
		return ERR_FAILURE;
	}

	size_t appidStrLen = snprintf(NULL, 0, "%d", appid) + 1;
	char appidStr[appidStrLen];
	sprintf(appidStr, "%d", appid);

	const char * path = g_hash_table_lookup(gamePaths, &gameId);
	char * loadOrderPath = g_build_filename(path, "steamapps/compatdata", appidStr, "pfx/drive_c/users/steamuser/AppData/Local/", GAMES_NAMES[gameId], "Plugins.txt", NULL);


	FILE * f_loadOrder = fopen(loadOrderPath, "w");
	while(loadOrder != NULL) {
		order_plugin_entry_t * entry = loadOrder->data;
		if(entry->activated) {
			fwrite("*", sizeof(char), 1, f_loadOrder);
			fwrite(entry->filename, sizeof(char), strlen(entry->filename), f_loadOrder);
			fwrite("\n", sizeof(char), 1, f_loadOrder);
		}
		loadOrder = g_list_next(loadOrder);
	}
	fclose(f_loadOrder);

	return ERR_SUCCESS;
}

//TODO: support compression since it can change how we read the file
//https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format#Records
//https://www.mwmythicmods.com/argent/tech/es_format.html
error_t order_getModDependencies(const char * esmPath, GList ** dependencies) {
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
		g_error( "Unrecognized file format %s\n", sectionName);
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
			char * dependency = g_malloc(subsectionLength + 1);
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

void order_free_plugin_entry(order_plugin_entry_t * entry) {
	free(entry->filename);
	g_free(entry);
}
