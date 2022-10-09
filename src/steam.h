#ifndef __STEAM_H__
#define __STEAM_H__

#include "macro.h"
#include "main.h"

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>

typedef struct steam_App {
	unsigned int appid;
	unsigned int update;
} steam_App_t;

typedef struct steam_Libraries {
	char * path;
	char * label;
	char * contentId;
	unsigned long totalSize;
	char * update_clean_bytes_tally;
	char * time_last_update_corruption;
	steam_App_t * apps;
	size_t appsCount;
} steam_Libraries_t;

//todo add the older games
// order has to be the same as in GAMES_NAMES
static const u_int32_t GAMES_APPIDS[] = {
	489830,
	22330,
	377160,
	22320
};

//the name of the game in the steamapps/common folder
static const char * GAMES_NAMES[] = {
	"Skyrim Special Edition",
	"Oblivion",
	"Fallout 4",
	"Morrowind"
};

_Static_assert(LEN(GAMES_APPIDS) == LEN(GAMES_NAMES), "Game APPIDS and Game Names doesn't match");

/**
 * @brief list all installed games and the paths to the game's files
 * This method has a singleton so after the first execution it will no rescan for new games.
 *
 * @param status pointer to a status variable that will be modified to EXIT_SUCCESS or EXIT_FAILURE
 * @return GHashTable* a map appid(int) => path(char *) to the corresponding steam library
 */
error_t steam_searchGames(GHashTable** tablePointer);

void steam_freeGameTable(void);

/**
 * @brief search the index of the game inside GAMES_NAMES or GAMES_APPIDS
 *
 * @param appid
 * @return -1 in case of failure or the index of the game.
 */
int steam_gameIdFromAppId(u_int32_t appid);

#endif
