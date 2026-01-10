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
	bool enabled;
} SteamLibrary_t;

//todo add the older games
// order has to be the same as in GAMES_NAMES
// 7 is enough to know if it's in GAMES_APPIDS no need to have the real value
#define GAMES_MAX_APPID_LENGTH 7

//sorted by appid, most of this data was aquired through old youtube modding tutorial and steamdb.info
static const u_int32_t GAMES_APPIDS[] = {
	489830,
	377160,
	72850,
	22380,
	22370,
	22330,
//	22320,
	22300
};

//the name of the game in the steamapps/common folder
static const char * GAMES_NAMES[] = {
	"Skyrim Special Edition",
	"Fallout 4",
	"skyrim",
	"Fallout New Vegas",
	"Fallout 3 goty",
	"Oblivion",
//	"Morrowind",
	"Fallout 3"
};

//should probably be done programically
static const char * GAMES_PREFIX_DIR[] = {
	"steamapps/compatdata/489830/pfx/",
	"steamapps/compatdata/377160/pfx/",
	"steamapps/compatdata/72850/pfx/",
	"steamapps/compatdata/22380/pfx/",
	"steamapps/compatdata/22370/pfx/",
	"steamapps/compatdata/22330/pfx/",
	"steamapps/compatdata/22300/pfx/"
};

static const char * GAMES_PLUGINS_FILE[] = {
	"drive_c/users/steamuser/AppData/Local/Skyrim Special Edition/Plugins.txt",
	"drive_c/users/steamuser/AppData/Local/Fallout 4/Plugins.txt", //Filled in assuming game name is identical and plugins is Plugins.txt
	"drive_c/users/steamuser/AppData/Local/skyrim/Plugins.txt", //Filled in assuming game name is identical and plugins is Plugins.txt
	"drive_c/users/steamuser/AppData/Local/FalloutNV/plugins.txt",
	"drive_c/users/steamuser/AppData/Local/Fallout 3/Plugins.txt", //Filled in assuming game name is identical and plugins is Plugins.txt
	"drive_c/users/steamuser/AppData/Local/Oblivion/Plugins.txt",
	//"steamapps/common/Morrowind/Morrowind.ini" Morrowind doesn't use Plugins.txt but Morrowind.ini the format is completely different. and is located within the game folder
	"drive_c/users/steamuser/AppData/Local/Fallout 3/Plugins.txt" //Filled in assuming game name is identical and plugins is Plugins.txt
};

//the directory in which mods will be installed.
//TODO: allow to override this so we can install skse/obse through the manager
static const char * GAMES_MOD_TARGET[] = {
	"Data",
	"Data",
	"Data",
	"Data",
	"Data",
	"Data",
//	"Data Files",
	"Data"
};

/*
static const char * GAMES_SCRIPT_EXTENDER_EXECUTABLE[] = {
	"skse64_loader.exe",
	"f4se_loader.exe",
	"skse_loader.exe",
	"nvse_loader.exe", //while it does have a steam_loader if the game has the 4gb patch it is required to use the exe.
	"fose_loader.exe",
	"obse_loader.exe", //the executable is not required for steam as obse_steam_loader.dll can load it dynamically but using it allows us to not have to fiddle with WINE_DLL_OVERRIDES.
	NULL, //TODO
	"fose_loader.exe"
};
*/

//Set the default executable to launch if the player didn't set another.
static const char * GAMES_EXECUTABLE[] = {
	"SkyrimSE.exe",
	"Fallout4.exe",
	"TESV.exe",
	"FalloutNV.exe",
	"Fallout3.exe",
	"Oblivion.exe",
//	"Morrowind.exe",
	"Fallout3.exe"
};

static_assert(sizeof(GAMES_EXECUTABLE) / sizeof(GAMES_EXECUTABLE[0]) == sizeof(GAMES_APPIDS) / sizeof(GAMES_APPIDS[0]),
	"Game APPIDS and GAMES_EXECUTABLE doesn't match");

/*
static_assert(sizeof(GAMES_SCRIPT_EXTENDER_EXECUTABLE) / sizeof(GAMES_SCRIPT_EXTENDER_EXECUTABLE[0]) == sizeof(GAMES_APPIDS) / sizeof(GAMES_APPIDS[0]),
	"Game APPIDS and GAMES_SCRIPT_EXTENDER_EXECUTABLE doesn't match");
*/

static_assert(sizeof(GAMES_PLUGINS_FILE) / sizeof(GAMES_PLUGINS_FILE[0]) == sizeof(GAMES_APPIDS) / sizeof(GAMES_APPIDS[0]),
	"Game APPIDS and GAMES_PLUGINS_FILE doesn't match");

static_assert(sizeof(GAMES_PREFIX_DIR) / sizeof(GAMES_PREFIX_DIR[0]) == sizeof(GAMES_APPIDS) / sizeof(GAMES_APPIDS[0]),
	"Game APPIDS and GAMES_PREFIX_DIR doesn't match");

static_assert(sizeof(GAMES_NAMES) / sizeof(GAMES_NAMES[0]) == sizeof(GAMES_APPIDS) / sizeof(GAMES_APPIDS[0]),
	"Game APPIDS and GAMES_NAMES doesn't match");

static_assert(sizeof(GAMES_MOD_TARGET) / sizeof(GAMES_MOD_TARGET[0]) == sizeof(GAMES_APPIDS) / sizeof(GAMES_APPIDS[0]),
	"Game mod target and GAMES_MOD_TARGET doesn't match");

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
