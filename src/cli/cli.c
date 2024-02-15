#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <libaudit.h>

#include <gameData.h>
#include <loadOrder.h>
#include <deploy.h>
#include <steam.h>
#include <getHome.h>
#include <steam.h>
#include <file.h>
#include <fomod.h>
#include <mods.h>
#include <constants.h>
#include <deploy.h>
#include <errorType.h>
#include <mods.h>

#include "cli.h"
#include "fomod.h"


#define ENABLED_COLOR "\033[0;32m"
#define DISABLED_FOMOD_COLOR "\033[0;31m"
#define ENABLED_FOMOD_COLOR "\033[0;33m"
#define DISABLED_COLOR "" //no color

static bool isRoot() {
	return getuid() == 0;
}

static int usage() {
	printf("Use --list-games or -l to list compatible games\n");

	printf("Use --bind or -d <APPID> to deploy the mods for the game\n");
	printf("Use --unbind <APPID> to rollback a deployment\n");

	printf("Use --add or -a <APPID> <FILENAME> to add a mod to a game\n");
	printf("Use --remove or -r <APPID> <MODID> to remove a mod\n");
	printf("Use --fomod <APPID> <MODID> to create a new mod using the result from the FOMod\n");

	printf("Use --list-mods or -m <APPID> to list all mods for a game\n");
	printf("Use --install or -i <APPID> <MODID> to add a mod to a game\n");
	printf("Use --uninstall or -u <APPID> <MODID> to uninstall a mod from a game\n");

	printf("Use --swap <APPID> <MODID A> MODID B> to swap two mod in the loading order\n");

	printf("Use --version or -v to get the version number\n");

	printf("Use --show-load-order <APPID>\n");
	printf("Use --list-plugins <APPID>\n");
	printf("Use --setup <APPID> to configure the game for the first time\n");
	return EXIT_FAILURE;
}

static int listGames(int argc, char **) {
	if(argc != 2) return usage();

	GHashTable * gamePaths;
	error_t status = steam_search_games(&gamePaths);
	if(status == ERR_FAILURE) {
		return EXIT_FAILURE;
	}

	GList * gamesIds = g_hash_table_get_keys(gamePaths);
	GList * gamesIdsFirstPointer = gamesIds;
	if(g_list_length(gamesIds) == 0) {
		printf("No game found\n");
	} else {
		while(gamesIds != NULL) {
			const int * gameIndex = (int*)gamesIds->data;
			printf("%d: %s\n", GAMES_APPIDS[*gameIndex], GAMES_NAMES[*gameIndex]);
			gamesIds = g_list_next(gamesIds);
		}
	}
	g_list_free(gamesIdsFirstPointer);
	return EXIT_FAILURE;
}

static int add(int argc, char ** argv) {
	if(argc != 4) return usage();
	const char * appIdStr = argv[2];

	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	GFile * file = g_file_new_for_path(argv[3]);
	error_t err = mods_add_mod(file, appid);
	g_free(file);
	return err;
}

static int listAllMods(int argc, char ** argv) {
	if(argc != 3) return usage();
	char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if( appid < 0) {
		return EXIT_FAILURE;
	}

	GList * mods = mods_list(appid);
	GList * p_mods = mods;
	unsigned short index = 0;

	printf("Id | Installed  | Name\n");
	while(mods != NULL) {
		char * modName = (char*)mods->data;
		mods_mod_detail_t details = mods_mod_details(appid, index);

		if(!details.has_fomodfile) {
			if(details.is_activated)
				printf(ENABLED_COLOR " %d | ✓ | %s\n", index, modName);
			else
				printf(DISABLED_COLOR " %d | ✕ | %s\n", index, modName);
		} else {
			if(details.is_activated)
				printf(ENABLED_FOMOD_COLOR " %d | ✓ | %s\n", index, modName);
			else
				printf(DISABLED_FOMOD_COLOR " %d | ✕ | %s\n", index, modName);
		}

		index++;
		mods = g_list_next(mods);
	}

	g_list_free_full(p_mods, free);
	return EXIT_SUCCESS;
}


/*
 * THIS WILL NOT INSTALL THE MOD
 * this will just flag the mod as needed to be deployed
 * it's the deploying process that handle the rest
*/
static int installAndUninstallMod(int argc, char ** argv, bool install) {
	if(argc != 4) return usage();
	char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	//strtoul set EINVAL if the string is invalid
	unsigned long modId = strtoul(argv[3], NULL, 10);
	if(errno == EINVAL) {
		fprintf(stderr, "ModId has to be a valid number\n");
		return -1;
	}

	mods_mod_detail_t details = mods_mod_details(appid, modId);
	if(!details.is_fomod && details.has_fomodfile) {
		printf("Warning: mod has a fomod file make sure to use --fomod before install");
	}

	error_t err;
	if(install) {
		err = mods_enable_mod(appid, modId);
	} else {
		err = mods_disable_mod(appid, modId);
	}
	return err != EXIT_SUCCESS;
}

static error_t cli_deploy(int argc, char ** argv) {
	if(argc != 3) return usage();

	char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	GList * ignoredMods = NULL;

	DeploymentErrors_t errors = deploy(appIdStr, &ignoredMods);
	switch(errors) {
	//we prevalidate the appid so it is a bug
	default:
	case INVALID_APPID:
	case BUG:
		fprintf(stderr, "A bug was detected during the deployment\n");
		return ERR_FAILURE;
	case CANNOT_MOUNT:
		fprintf(stderr, "Failed to mount the game files\n");
		return ERR_FAILURE;
	case FUSE_NOT_INSTALLED:
		fprintf(stderr, "This software requires fuse-overlayfs to be installed\n");
		return ERR_FAILURE;
	case GAME_NOT_FOUND:
		fprintf(stderr, "Could not find the game files\n");
		return ERR_FAILURE;
	case OK:
		printf("Success\n");
		return ERR_SUCCESS;
	}
}

static int setup(int argc, char ** argv) {
	if(argc != 3 ) return usage();
	const char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}
	g_autofree GFile * home = audit_get_home();
	g_autofree char * home_path = g_file_get_path(home);

	g_autofree GFile * gameUpperDir = g_file_new_build_filename(home_path, MODLIB_WORKING_DIR, GAME_UPPER_DIR_NAME, appIdStr, NULL);
	g_autofree GFile * gameWorkDir = g_file_new_build_filename(home_path, MODLIB_WORKING_DIR, GAME_WORK_DIR_NAME, appIdStr, NULL);
	g_file_make_directory_with_parents(gameUpperDir, NULL, NULL);
	g_file_make_directory_with_parents(gameWorkDir, NULL, NULL);

	GFile * dataFolder = NULL;
	error_t error = game_data_get_data_path(appid, &dataFolder);
	if(error != ERR_SUCCESS) {
		return ERR_FAILURE;
	}

	g_autofree GFile * gameFolder = g_file_new_build_filename(home_path, MODLIB_WORKING_DIR, GAME_FOLDER_NAME, appIdStr, NULL);
	if(g_file_query_exists(gameFolder, NULL)) {
		//if the game folder alredy exists just delete it
		//this will allow the removal of dlcs and language change
		file_delete_recursive(gameFolder, NULL, NULL);
	}
	//TODO: error checking
	g_file_make_directory_with_parents(gameFolder, NULL, NULL);

	//links don't conflict with overlayfs and avoid coping 17Gb of files.
	//but links require the files to be on the same filesystem
	if(file_recursive_copy(dataFolder, gameFolder, G_FILE_COPY_NONE, NULL, NULL) != ERR_SUCCESS) {
		printf("Coping game files.  HINT: having the game on the same partition as you home director will make this operation use zero extra space\n");
		return EXIT_FAILURE;
	}
	file_casefold(gameFolder);

	g_free(dataFolder);
	printf("Done\n");
	return EXIT_SUCCESS;
}


static int unbind(int argc, char ** argv) {
	if(argc != 3 ) return usage();
	const char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	GFile * data_folder_file = NULL;
	error_t error = game_data_get_data_path(appid, &data_folder_file);
	if(error != ERR_SUCCESS) {
		return ERR_FAILURE;
	}

	g_autofree char * data_folder = g_file_get_path(data_folder_file);

	while(umount2(data_folder, MNT_FORCE | MNT_DETACH) == 0);

	return EXIT_SUCCESS;
}

static int removeMod(int argc, char ** argv) {
	if(argc != 4) return usage();
	const char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		fprintf(stderr, "Invalid appid\n");
		return EXIT_FAILURE;
	}

	//strtoul set EINVAL if the string is invalid
	unsigned long modId = strtoul(argv[3], NULL, 10);
	if(errno == EINVAL) {
		fprintf(stderr, "Modid has to be a valid number\n");
		return EXIT_FAILURE;
	}

	return mods_remove_mod(appid, modId);
}

static int fomod(int argc, char ** argv) {
	if(argc != 4) return usage();
	char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		fprintf(stderr, "Invalid appid");
		return EXIT_FAILURE;
	}

	//strtoul set EINVAL if the string is invalid
	unsigned long modId = strtoul(argv[3], NULL, 10);
	if(errno == EINVAL) {
		fprintf(stderr, "Modid has to be a valid number\n");
		return -1;
	}

	//might crash if no mods were installed
	g_autofree GFile * home = audit_get_home();
	g_autofree char * home_path = g_file_get_path(home);
	g_autofree char * modFolder = g_build_filename(home_path, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appIdStr, NULL);
	GList * mods = mods_list(appid);
	GList * modsFirstPointer = mods;

	for(unsigned long i = 0; i < modId; i++) {
		mods = g_list_next(mods);
	}

	if(mods == NULL) {
		fprintf(stderr, "Mod not found\n");
		return EXIT_FAILURE;
	}

	g_autofree char * destination = g_strconcat(mods->data, "__FOMOD", NULL);
	g_autofree GFile * destination_file = g_file_new_build_filename(destination, NULL);

	if(g_file_query_exists(destination_file, NULL)) {
		//TODO: add error handling
		file_delete_recursive(destination_file, NULL, NULL);
	}
	g_autofree GFile * modDestination = g_file_new_build_filename(modFolder, destination, NULL);

	//TODO: add error handling
	int returnValue = fomod_installFOMod(mods->data, modDestination);

	g_list_free_full(modsFirstPointer, free);
	return returnValue;
}

static int swapMod(int argc, char ** argv) {
	if(argc != 5) return usage();
	char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		fprintf(stderr, "Invalid appid\n");
		return EXIT_FAILURE;
	}


	char * sentinel;
	//no mod will go over the MAX_INT value
	int modIdA = (int)strtoul(argv[3], &sentinel, 10);
	if(errno == EINVAL || sentinel == argv[3]) {
		fprintf(stderr, "Modid A has to be a valid number\n");
		return -1;
	}
	int modIdB = (int)strtoul(argv[4], &sentinel, 10);
	if(errno == EINVAL || sentinel == argv[4]) {
		fprintf(stderr, "Modid B has to be a valid number\n");
		return -1;
	}

	printf("%d, %d\n", modIdA, modIdB);
	return mods_swap_place(appid, modIdA, modIdB);
}

static int printLoadOrder(int argc, char ** argv) {
	if(argc != 3) return usage();

	char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		fprintf(stderr, "Invalid appid\n");
		return EXIT_FAILURE;
	}

	GList * order = NULL;
	error_t status = order_get_load_order(appid, &order);
	if(status != ERR_SUCCESS) {
		fprintf(stderr, "Could not fetch the load order\n");
		return EXIT_FAILURE;
	}

	printf("If the load order is empty that mean it wasn't set\n");

	GList * order_iterator = order;
	while (order_iterator != NULL) {
		const order_plugin_entry_t * details = (const order_plugin_entry_t *)order_iterator->data;
		printf("name: %s active: %s\n", details->filename, details->activated ? "✓" : "✕");
		order_iterator = g_list_next(order_iterator);
	}

	g_list_free_full(order, (GDestroyNotify)order_free_plugin_entry);
	return EXIT_SUCCESS;
}

static int printPlugins(int argc, char ** argv) {
	if(argc != 3) return usage();

	char * appIdStr = argv[2];
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		fprintf(stderr, "Invalid appid\n");
		return EXIT_FAILURE;
	}

	GList * mods = NULL;
	error_t status = order_listPlugins(appid, &mods);
	if(status != ERR_SUCCESS) {
		fprintf(stderr, "Could not list plugins\n");
		return EXIT_FAILURE;
	}

	GList * cur_mods = mods;
	while (cur_mods != NULL) {
		printf("%s\n", (char *)cur_mods->data);
		cur_mods = g_list_next(cur_mods);
	}

	g_list_free_full(mods, free);
	return EXIT_SUCCESS;
}

int main(int argc, char ** argv) {
	if(argc < 2 ) return usage();

	if(audit_getloginuid() == 0 || isRoot()) {
		fprintf(stderr, "Please don't run this software as root\n");
		return EXIT_FAILURE;
	}

	int returnValue = EXIT_SUCCESS;
	GFile * home = audit_get_home();
	char * home_path = g_file_get_path(home);
	char * configFolder = g_build_filename(home_path, MODLIB_WORKING_DIR, NULL);
	g_free(home);
	g_free(home_path);

	if(access(configFolder, F_OK) != 0) {

		//check to NOT run this as root
		//creating folder as root will causes lots of headache
		//such as problem with access rights and taking the risk of
		//running system as root (which is a big security issue)
		if(getuid() == 0) {
			fprintf(stderr, "For the first run please avoid sudo\n");
			returnValue = EXIT_FAILURE;
			goto exit;
		} else {
			//leading 0 == octal
			int i = g_mkdir_with_parents(configFolder, 0755);
			if(i < 0) {
				fprintf(stderr, "Could not create configs, check access rights for this path: %s", configFolder);
				goto exit;
			}
		}
	}

	if(strcmp(argv[1], "--list-games") == 0 || strcmp(argv[1], "-l") == 0)
		returnValue = listGames(argc, argv);

	else if(strcmp(argv[1], "--add") == 0 || strcmp(argv[1], "-a") == 0)
		returnValue = add(argc, argv);

	else if(strcmp(argv[1], "--list-mods") == 0 || strcmp(argv[1], "-m") == 0)
		returnValue = listAllMods(argc, argv);

	else if(strcmp(argv[1], "--install") == 0 || strcmp(argv[1], "-i") == 0)
		returnValue = installAndUninstallMod(argc, argv, true);

	else if(strcmp(argv[1], "--uninstall") == 0 || strcmp(argv[1], "-u") == 0)
		returnValue = installAndUninstallMod(argc, argv, false);

	else if(strcmp(argv[1], "--bind") == 0 || strcmp(argv[1], "-d") == 0)
		returnValue = cli_deploy(argc, argv);

	else if(strcmp(argv[1], "--unbind") == 0)
		returnValue = unbind(argc, argv);

	else if(strcmp(argv[1], "--setup") == 0)
		returnValue = setup(argc, argv);

	else if(strcmp(argv[1], "--fomod") == 0)
		returnValue = fomod(argc, argv);

	else if(strcmp(argv[1], "--remove") == 0 || strcmp(argv[1], "-r") == 0)
		returnValue = removeMod(argc, argv);

	else if(strcmp(argv[1], "--swap") == 0 || strcmp(argv[1], "-s") == 0)
		returnValue = swapMod(argc, argv);

	else if(strcmp(argv[1], "--list-plugins") == 0)
		returnValue = printPlugins(argc, argv);

	else if(strcmp(argv[1], "--show-load-order") == 0)
		returnValue = printLoadOrder(argc, argv);

	else if(strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
		#ifdef __clang__
			printf("%s: Clang: %d.%d.%d\n", VERSION, __clang_major__, __clang_minor__, __clang_patchlevel__);
		#elifdef __GNUC__
			printf("%s: GCC: %d.%d.%d\n", VERSION, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
		#else
			printf("%s: unknown compiler\n", VERSION);
		#endif
	} else {
		usage();
		returnValue = EXIT_FAILURE;
	}

exit:
	g_free(configFolder);
	steam_freeGameTable();
	return returnValue;
}
