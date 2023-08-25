
#include <constants.h>
#include <gameData.h>
#include <steam.h>

#include <glib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

error_t gameData_get_data_path(int appid, GFile ** destination) {
    GHashTable * gamePaths;
	error_t status = steam_searchGames(&gamePaths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	int gameId = steam_gameIdFromAppId(appid);
	if(gameId < 0 ) {
		g_error( "invalid appid");
		return ERR_FAILURE;
	}

	const char * path = g_hash_table_lookup(gamePaths, &gameId);

	if(path == NULL) {
		g_error( "game not found\n");
		return ERR_FAILURE;
	}

	g_autofree char * gameFolder = g_build_filename(path, "steamapps/common", GAMES_NAMES[gameId], NULL);
	//the folder names for older an newer titles
	GFile * data_folder_old = g_file_new_build_filename(gameFolder, "Data Files", NULL);
	GFile * data_folder_new = g_file_new_build_filename(gameFolder, "Data", NULL);

	*destination = NULL;
	//struct stat sb;
	if(g_file_query_exists(data_folder_old, NULL)) {
		*destination = data_folder_old;
		g_free(data_folder_new);
	} else if(g_file_query_exists(data_folder_new, NULL)) {
		*destination = data_folder_new;
		g_free(data_folder_old);
	} else {
		g_free(data_folder_new);
		g_free(data_folder_old);
	}

	if(*destination == NULL) return ERR_FAILURE;
	else return ERR_SUCCESS;
}
