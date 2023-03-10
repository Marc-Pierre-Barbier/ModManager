
#include <constants.h>
#include <gameData.h>
#include <steam.h>

#include <glib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

error_t gameData_getDataPath(int appid, char ** destination) {
    GHashTable * gamePaths;
	error_t status = steam_searchGames(&gamePaths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	int gameId = steam_gameIdFromAppId(appid);
	if(gameId < 0 ) {
		fprintf(stderr, "invalid appid");
		return ERR_FAILURE;
	}

	const char * path = g_hash_table_lookup(gamePaths, &gameId);

	if(path == NULL) {
		fprintf(stderr, "game not found\n");
		return ERR_FAILURE;
	}

	char * gameFolder = g_build_filename(path, "steamapps/common", GAMES_NAMES[gameId], NULL);
	//the folder names for older an newer titles
	char * dataFolderOld = g_build_filename(gameFolder, "Data Files", NULL);
	char * dataFolderNew = g_build_filename(gameFolder, "Data", NULL);

	g_free(gameFolder);

	*destination = NULL;
	//struct stat sb;
	if(access(dataFolderOld, F_OK) == 0) {
		*destination = strdup(dataFolderOld);
	} else if(access(dataFolderNew, F_OK) == 0) {
		*destination = strdup(dataFolderNew);
	}

	g_free(dataFolderNew);
	g_free(dataFolderOld);

	if(*destination == NULL) return ERR_FAILURE;
	else return ERR_SUCCESS;
}
