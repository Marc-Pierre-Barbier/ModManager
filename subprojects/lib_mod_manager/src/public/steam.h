#ifndef __STEAM_H__
#define __STEAM_H__

#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>
#include <gio/gio.h>
#include "errorType.h"

typedef struct SteamApp {
	unsigned int appid;
	unsigned int update;
} SteamApp_t;

typedef struct SteamLibrary {
	char * path;
	char * label;
	char * contentId;
	unsigned long totalSize;
	char * update_clean_bytes_tally;
	char * time_last_update_corruption;
	SteamApp_t * apps;
	size_t apps_count;
} SteamLibrary_t;

//todo add the older games
// order has to be the same as in GAMES_NAMES
// 7 is enough to know if it's in GAMES_APPIDS no need to have the real value
#define GAMES_MAX_APPID_LENGTH 7

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

//the directory in which mods will be installed.
//TODO: allow to override this so we can install skse/obse through the manager
static const char * GAMES_MOD_TARGET[] = {
	"Data",
	"Data",
	"Data",
	"Data Files"
};

_Static_assert(sizeof(GAMES_NAMES) / sizeof(GAMES_NAMES[0]) == sizeof(GAMES_APPIDS) / sizeof(GAMES_APPIDS[0]),
	"Game APPIDS and Game Names doesn't match");

_Static_assert(sizeof(GAMES_MOD_TARGET) / sizeof(GAMES_MOD_TARGET[0]) == sizeof(GAMES_APPIDS) / sizeof(GAMES_APPIDS[0]),
	"Game mod target and Game Names doesn't match");

/**
 * @brief list all installed games and the paths to the game's files
 * This method has a singleton so after the first execution it will no rescan for new games.
 *
 * @param status pointer to a status variable that will be modified to EXIT_SUCCESS or EXIT_FAILURE
 * @return GHashTable* a map gameId(int) => path(char *) to the corresponding steam library
 */
error_t steam_search_games(GHashTable** table_pointer);

void steam_freeGameTable(void);

/**
 * @brief search the index of the game inside GAMES_NAMES or GAMES_APPIDS
 *
 * @param appid
 * @return -1 in case of failure or the index of the game.
 */
int steam_game_id_from_app_id(u_int32_t appid);

/**
 * @brief Parse the steamId string
 *
 * @param appIdStr steamId string
 * @return int -1 if failures or an int containing the steamId
 */
int steam_parseAppId(const char * app_id_str);


/**
 * @brief Find the game folder
 *
 * @param appid game app id
 * @return GFile* can be null
 */
GFile * steam_get_game_folder_path(int appid);

#endif
