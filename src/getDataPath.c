
#include "getDataPath.h"
#include "main.h"
#include "steam.h"

#include <glib.h>
#include <unistd.h>

error_t getDataPath(int appid, char ** destination) {
    GHashTable * gamePaths;
	error_t status = search_games(&gamePaths);
	if(status == ERR_FAILURE) {
		return ERR_FAILURE;
	}

	int gameId = getGameIdFromAppId(appid);
	if(gameId < 0 ) {
		return ERR_FAILURE;
	}

    const char * path = g_hash_table_lookup(gamePaths, &appid);
	char * gameFolder = g_build_filename(path, "steamapps/common", GAMES_NAMES[gameId], NULL);
	//the folder names for older an newer titles
	char * dataFolderOld = g_build_filename(gameFolder, "Data Files", NULL);
	char * dataFolderNew = g_build_filename(gameFolder, "Data", NULL);
	g_free(gameFolder);

	*destination = NULL;
	if(access(dataFolderOld, F_OK) == 0) {
		*destination = strdup(dataFolderOld);
	} else if(access(dataFolderNew, F_OK) == 0) {
		*destination = strdup(dataFolderOld);
	}

	g_free(dataFolderNew);
	g_free(dataFolderOld);

	if(*destination == NULL) return ERR_FAILURE;
	else return ERR_SUCCESS;
}
