#ifndef __STEAM_H__
#define __STEAM_H__

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>

typedef struct ValveApp {
	unsigned int appid;
	unsigned int update;
} ValveApp_t;

typedef struct ValveLibraries {
	char * path;
	char * label;
	char * contentId;
	unsigned long totalSize;
	char * update_clean_bytes_tally;
	char * time_last_update_corruption;
	ValveApp_t * apps;
	size_t appsCount;
} ValveLibraries_t;

// relative to the home directory
const static char * steamLibraries[] = {
	"/.steam/root/",
	"/.steam/steam/",
	"/.local/share/steam",
	//flatpack steam.
	"/.var/app/com.valvesoftware.Steam/.local/share/Steam"
};

//todo add the older games
// order has to be the same as in GAMES_NAMES
const static u_int32_t GAMES_APPIDS[] = {
	489830,
	22330,
	377160
};

//the name of the game in the steamapps/common folder
const static char * GAMES_NAMES[] = {
	"Skyrim Special Edition",
	"Oblivion",
	"Fallout 4"
};

/**
 * @brief list all installed games and the paths to the game's files
 *
 * @param status pointer to a status variable that will be modified to EXIT_SUCCESS or EXIT_FAILURE
 * @return GHashTable* a map appid(int) => path(char *)
 */
 //TODO: flip the return value and parameter
GHashTable* search_games(int * status);

/**
 * @brief search the index of the game inside GAMES_NAMES or GAMES_APPIDS
 *
 * @param appid
 * @return -1 in case of failure or the index of the game.
 */
int getGameIdFromAppId(u_int32_t appid);

#endif
