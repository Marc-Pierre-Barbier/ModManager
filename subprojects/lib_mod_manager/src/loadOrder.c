#include <constants.h>

#include "loadOrder.h"
#include "steam.h"
#include "file.h"

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <steam.h>

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
	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	const int game_id = steam_game_id_from_app_id(appid);
	if(game_id == -1) {
		return ERR_FAILURE;
	}

	GFile * game_folder_file = steam_get_game_folder_path(appid);
	if(game_folder_file == NULL) {
		return ERR_FAILURE;
	}
	g_autofree char * game_folder = g_file_get_path(game_folder_file);
	g_autofree GFile * data_folder_file = g_file_new_build_filename(game_folder, GAMES_MOD_TARGET[game_id], NULL);

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

	return ERR_SUCCESS;
}

static void remove_line_ending(char * string) {
	int size = strlen(string);
	if(string[size - 1] == '\r') size--;
	if(string[size - 1] == '\n') size--;
	if(string[size - 1] == '\r') size--;
	string[size] = '\0';
}

error_t order_get_load_order(int appid, GList ** order) {

	GHashTable * game_paths;
	error_t status = steam_search_games(&game_paths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	GList * l_plugins = NULL;
	error_t error = order_listPlugins(appid, &l_plugins);
	if(error == ERR_FAILURE)
		return ERR_FAILURE;


	int gameId = steam_game_id_from_app_id(appid);
	if(gameId < 0 ) {
		return ERR_FAILURE;
	}

	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	const char * path = g_hash_table_lookup(game_paths, &gameId);


	//this is the path i would use in windows but it seems it is not avaliable in all wine versions.
	//char * loadOrderPath = g_build_filename(path, "steamapps/compatdata", appidStr, "pfx/drive_c/users/steamuser/AppData/Local/", GAMES_NAMES[gameId], "loadorder.txt", NULL);
	g_autofree char * load_order_path = g_build_filename(path, "steamapps/compatdata", appid_str, "pfx/drive_c/users/steamuser/Local Settings/Application Data/", GAMES_NAMES[gameId], "Plugins.txt", NULL);

	GList * l_current_load_order = NULL;
	if(access(load_order_path, R_OK) == 0) {
		FILE * f_load_order = fopen(load_order_path, "r");

		size_t length = 0;
		char * line = NULL;

		while(getline(&line, &length, f_load_order) > 0) {
			if(line[0] == '#' || line[0] == '\n')
				continue;

			remove_line_ending(line);

			l_current_load_order = g_list_append(l_current_load_order, strdup(line));
		}
		if(line != NULL)free(line);
	}


	GList * l_current_load_order_iterator = l_current_load_order;
	GList * l_complete_load_order = NULL;

	while(l_current_load_order_iterator != NULL) {
		//pointer arithmetic to skip the *
		const char * mod_name = (const char *)l_current_load_order_iterator->data + 1;

		GList * mod = g_list_find_custom(l_plugins, mod_name, (GCompareFunc)strcmp);
		if(mod != NULL) {
			order_plugin_entry_t * new_entry = g_malloc(sizeof(order_plugin_entry_t));
			new_entry->filename = strdup(mod_name);
			new_entry->activated = TRUE;
			l_complete_load_order = g_list_append(l_complete_load_order, new_entry);
		}
		l_current_load_order_iterator = g_list_next(l_current_load_order_iterator);
	}

	GList * l_plugins_iterator = l_plugins;
	while(l_plugins_iterator != NULL) {
		const char * filename = (const char *)l_plugins_iterator->data;

		GList * mod = g_list_find_custom(l_complete_load_order, filename, order_find_file_with_name);
		if(mod == NULL) {
			order_plugin_entry_t * new_entry = g_malloc(sizeof(order_plugin_entry_t));
			new_entry->filename = strdup(filename);
			new_entry->activated = FALSE;
			l_complete_load_order = g_list_append(l_complete_load_order, new_entry);
		}

		l_plugins_iterator = g_list_next(l_plugins_iterator);
	}

	*order = l_complete_load_order;

	g_list_free_full(l_plugins, g_free);
	g_list_free_full(l_current_load_order, g_free);
	return ERR_SUCCESS;
}

//TODO: Check if the default LF format is compatible with windows' CRLF in skyrim
//TODO: add loadorder.txt along with plugins.txt
error_t order_set_load_order(int appid, GList * loadOrder) {
	GHashTable * game_paths;
	error_t status = steam_search_games(&game_paths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	int gameId = steam_game_id_from_app_id(appid);
	if(gameId < 0 ) {
		return ERR_FAILURE;
	}

	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	const char * path = g_hash_table_lookup(game_paths, &gameId);
	g_autofree char * load_order_path = g_build_filename(path, "steamapps/compatdata", appid_str, "pfx/drive_c/users/steamuser/AppData/Local/", GAMES_NAMES[gameId], NULL);
	g_autofree char * plugin_txt_path = g_build_filename(load_order_path, "Plugins.txt", NULL);

	g_mkdir_with_parents(load_order_path, 755);
	FILE * f_loadOrder = fopen(plugin_txt_path, "w");
	if(f_loadOrder == NULL) {
		return ERR_FAILURE;
	}

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
error_t order_get_mod_dependencies(const char * esm_path, GList ** dependencies) {
	FILE * file = fopen(esm_path, "r");

	char section_name[5];
	section_name[4] = '\0';
	fread(section_name, sizeof(char), 4, file);

	size_t size_field_size = 0;
	u_int8_t record_header_to_ignore = 0;

	//the field "length" in the sub-record change between games.
	//TES4 => Fallout4 Oblivion Skyrim(+SE)
	//TES3 => Morrowind
	if(strcmp(section_name, "TES3") == 0) {
		printf("Using tes3 file format\n");
		record_header_to_ignore = 8;
		size_field_size = 4;
	} else if(strcmp(section_name, "TES4") == 0) {
		printf("Using tes4 file format\n");
		record_header_to_ignore = 16;
		size_field_size = 2;
	} else {
		g_warning( "Unrecognized file format %s\n", section_name);
		fclose(file);
		return ERR_FAILURE;
	}


	u_int32_t length_val = 0;
	fread(&length_val, 4, 1, file);
	//ignore the rest of the data
	fseek(file, record_header_to_ignore, SEEK_CUR);

	int64_t length = length_val;
	while(length > 0) {
		char section_name[5];
		section_name[4] = '\0';
		fread(section_name, sizeof(char), 4, file);
		unsigned long subsection_length = 0;
		fread(&subsection_length, size_field_size, 1, file);

		length -= 8 + subsection_length;
		if(strcmp(section_name, "MAST") == 0) {
			char * dependency = g_malloc(subsection_length + 1);
			dependency[subsection_length] = '\0';
			fread(dependency, sizeof(char), subsection_length, file);
			*dependencies = g_list_append(*dependencies, dependency);
		} else {
			fseek(file, subsection_length, SEEK_CUR);
		}
	}
	fclose(file);
	return ERR_SUCCESS;
}

void order_free_plugin_entry(order_plugin_entry_t * entry) {
	free(entry->filename);
	g_free(entry);
}
