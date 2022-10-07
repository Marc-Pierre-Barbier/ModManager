#include <stdbool.h>
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <libaudit.h>

#include "overlayfs.h"
#include "install.h"
#include "getHome.h"
#include "steam.h"
#include "main.h"
#include "file.h"
#include "fomod.h"
#include "order.h"

#define ENABLED_COLOR "\033[0;32m"
#define DISABLED_COLOR "\033[0;31m"

GHashTable * gamePaths;

static bool isRoot() {
	return getuid() == 0;
}

static GList * listFilesInFolder(const char * path) {
	GList * list = NULL;
	DIR *d;
	struct dirent *dir;
	d = opendir(path);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			//removes .. & . from the list
			if(strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {
				list = g_list_append(list, strdup(dir->d_name));
			}
		}
		closedir(d);
	}

	return list;
}

static int usage() {
	printf("Use --list-games or -l to list compatible games\n");
	printf("Use --add or -a <APPID> <FILENAME> to add a mod to a game\n");
	printf("Use --list-mods or -m <APPID> to list all mods for a game\n");
	printf("Use --install or -i <APPID> <MODID> to add a mod to a game\n");
	printf("Use --uninstall or -u <APPID> <MODID> to uninstall a mod from a game\n");
	printf("Use --remove or -r <APPID> <MODID> to remove a mod\n");
	printf("Use --deploy or -d <APPID> to deploy the mods for the game\n");
	printf("Use --unbind <APPID> to rollback a deployment\n");
	printf("Use --fomod <APPID> <MODID> to create a new mod using the result from the FOMod\n");
	printf("Use --swap <APPID> <MODID A> MODID B> to swap two mod in the loading order\n");
	printf("Use --version or -v to get the version number\n");
	return EXIT_FAILURE;
}

static int validateAppId(const char * appIdStr) {
	char * strtoulSentinel;
	//strtoul set EINVAL(after C99) if the string is invalid
	unsigned long appid = strtoul(appIdStr, &strtoulSentinel, 10);
	if(errno == EINVAL || strtoulSentinel == appIdStr) {
		fprintf(stderr, "Appid has to be a valid number\n");
		return -1;
	}

	int gameId = getGameIdFromAppId((int)appid);
	if(gameId < 0) {
		fprintf(stderr, "Game is not compatible\n");
		return -1;
	}

	if(!g_hash_table_contains(gamePaths, &gameId)) {
		fprintf(stderr, "Game not found\n");
		return -1;
	}

	//no valid appid goes far enough to justify long
	return (int)appid;
}

static int listGames(int argc, char **) {
	if(argc != 2) return usage();
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
	return EXIT_SUCCESS;
}

static int add(int argc, char ** argv) {
	if(argc != 4) return usage();
	const char * appIdStr = argv[2];

	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	addMod(argv[3], appid);
	return EXIT_SUCCESS;
}

static int listAllMods(int argc, char ** argv) {
	if(argc != 3) return usage();
	char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
	if( appid < 0) {
		return EXIT_FAILURE;
	}

	//might crash if no mods were installed
	char * home = getHome();
	char * modFolder = g_build_filename(home, MANAGER_FILES, MOD_FOLDER_NAME, appIdStr, NULL);
	free(home);

	GList * mods = listMods(appid);
	GList * p_mods = mods;
	unsigned short index = 0;

	printf("Id | Installed  | Name\n");
	while(mods != NULL) {
		char * modName = (char*)mods->data;
		char * modPath = g_build_filename(modFolder, modName, INSTALLED_FLAG_FILE, NULL);

		if(access(modPath, F_OK) == 0) {
			printf(ENABLED_COLOR " %d | ✓ | %s\n", index, modName);
		} else {
			printf(DISABLED_COLOR " %d | ✕ | %s\n", index, modName);
		}

		g_free(modPath);

		index++;
		mods = g_list_next(mods);
	}


	free(modFolder);
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
	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	int returnValue = EXIT_SUCCESS;

	//strtoul set EINVAL if the string is invalid
	unsigned long modId = strtoul(argv[3], NULL, 10);
	if(errno == EINVAL) {
		fprintf(stderr, "ModId has to be a valid number\n");
		return -1;
	}

	//might crash if no mods were installed
	char * home = getHome();
	char * modFolder = g_build_filename(home, MANAGER_FILES, MOD_FOLDER_NAME, appIdStr, NULL);
	free(home);
	GList * mods = listFilesInFolder(modFolder);
	GList * modsFirstPointer = mods;

	for(unsigned long i = 0; i < modId; i++) {
		mods = g_list_next(mods);
	}

	if(mods == NULL) {
		fprintf(stderr, "Mod not found\n");
		return EXIT_FAILURE;
	}

	char * modName = (char*)mods->data;
	char * modFlag = g_build_filename(modFolder, modName, INSTALLED_FLAG_FILE, NULL);

	if(install) {
		if(access(modFlag, F_OK) == 0) {
			fprintf(stderr, "The mod is already activated \n");
			returnValue = EXIT_SUCCESS;
			goto exit;
		}

		//Create activated file
		FILE * fd = fopen(modFlag, "w+");
		if(fd != NULL) {
			fwrite("", 1, 1, fd);
			fclose(fd);
		} else {
			printf("Could not interact with the activation file\n");
			returnValue = EXIT_FAILURE;
		}
	} else {
		if(access(modFlag, F_OK) != 0) {
			printf("The mod is not activated \n");
			returnValue = EXIT_SUCCESS;
			goto exit;
		}

		if(unlink(modFlag) != 0) {
			fprintf(stderr, "Error could not disable the mod.\n");
			fprintf(stderr, "please remove this file '%s'\n", modFlag);
			returnValue = EXIT_FAILURE;
			goto exit;
		}
	}

exit:

	g_free(modFolder);
	g_list_free_full(modsFirstPointer, free);
	g_free(modFlag);
	return returnValue;
}

static int deploy(int argc, char ** argv) {
	if(argc != 3) return usage();

	char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	char * home = getHome();
	char * gameFolder = g_build_filename(home, MANAGER_FILES, GAME_FOLDER_NAME, appIdStr, NULL);

	if(access(gameFolder, F_OK) != 0) {
		free(home);
		g_free(gameFolder);
		printf("Please run '%s --setup %d' first", APP_NAME, appid);
		return EXIT_FAILURE;
	}

	//unmount everything beforhand
	//might crash if no mods were installed
	char * modFolder = g_build_filename(home, MANAGER_FILES, MOD_FOLDER_NAME, appIdStr, NULL);

	GList * mods = listFilesInFolder(modFolder);
	//since the priority is by alphabetical order
	//and we need to bind the least important first
	//and the most important last
	//we have to reverse the list
	mods = g_list_reverse(mods);

	//probably over allocating but this doesn't matter that much
	char * modsToInstall[g_list_length(mods) + 2];
	int modCount = 0;

	while(true) {
		if(mods == NULL)break;
		char * modName = (char *)mods->data;
		char * modPath = g_build_path("/", modFolder, modName, NULL);
		char * modFlag = g_build_filename(modPath, INSTALLED_FLAG_FILE, NULL);

		//if the mod is not marked to be deployed then don't
		if(access(modFlag, F_OK) != 0) {
			printf("ignoring mod %s\n", modName);
			mods = g_list_next(mods);
			continue;
		}

		//this is made to leave the first case empty so i can put lowerdir in it before feeding it to g_strjoinv
		printf("Installing %s\n", modName);
		modsToInstall[modCount] = modPath;
		modCount += 1;

		mods = g_list_next(mods);
		free(modFlag);
	}

	g_free(modFolder);

	//const char * data = "lowerdir=gameFolder,gameFolder2,gameFolder3..."
	modsToInstall[modCount] = gameFolder;
	modsToInstall[modCount + 1] = NULL;
	printf("Mounting the overlay\n");
	int gameId = getGameIdFromAppId(appid);
	const char * path = g_hash_table_lookup(gamePaths, &gameId);
	char * steamGameFolder = g_build_path("/", path, "steamapps/common", GAMES_NAMES[gameId], "Data", NULL);

	char * gameUpperDir = g_build_filename(home, MANAGER_FILES, GAME_UPPER_DIR_NAME, appIdStr, NULL);
	char * gameWorkDir = g_build_filename(home, MANAGER_FILES, GAME_WORK_DIR_NAME, appIdStr, NULL);
	free(home);

	//unmount the game folder
	//DETACH + FORCE allow us to be sure it will be unload.
	//it might crash / corrupt game file if the user do it while the game is running
	//but it's still very unlikely
	while(umount2(steamGameFolder, MNT_FORCE | MNT_DETACH) == 0);
	enum overlayErrors status = overlayMount(modsToInstall, steamGameFolder, gameUpperDir, gameWorkDir);
	if(status == SUCESS) {
		printf("Everything is ready, just launch the game\n");
	} else if(status == FAILURE) {
		fprintf(stderr, "Could not mount the mods overlay\n");
		return EXIT_FAILURE;
	} else if(status == NOT_INSTALLED) {
		fprintf(stderr, "Please install fuse-overlayfs\n");
		return EXIT_FAILURE;
	} else {
		fprintf(stderr, "%d bug detected, please report this.\n", __LINE__);
		return EXIT_FAILURE;
	}

	g_free(steamGameFolder);
	g_free(gameUpperDir);
	g_free(gameWorkDir);

	return EXIT_SUCCESS;
}

static int setup(int argc, char ** argv) {
	if(argc != 3 ) return usage();
	const char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}
	int gameId = getGameIdFromAppId(appid);
	char * home = getHome();

	char * gameUpperDir = g_build_filename(home, MANAGER_FILES, GAME_UPPER_DIR_NAME, appIdStr, NULL);
	char * gameWorkDir = g_build_filename(home, MANAGER_FILES, GAME_WORK_DIR_NAME, appIdStr, NULL);
	g_mkdir_with_parents(gameUpperDir, 0755);
	g_mkdir_with_parents(gameWorkDir, 0755);
	free(gameUpperDir);
	free(gameWorkDir);

	const char * path = g_hash_table_lookup(gamePaths, &gameId);
	char * steamGameFolder = g_build_path("/", path, "steamapps/common", GAMES_NAMES[gameId], "Data", NULL);

	char * gameFolder = g_build_filename(home, MANAGER_FILES, GAME_FOLDER_NAME, appIdStr, NULL);
	if(access(gameFolder, F_OK) == 0) {
		//if the game folder alredy exists just delete it
		//this will allow the removal of dlcs and language change
		delete(gameFolder, true);
	}
	g_mkdir_with_parents(gameFolder, 0755);

	//links don't conflict with overlayfs and avoid coping 17Gb of files.
	//but links require the files to be on the same filesystem
	int returnValue = copy(steamGameFolder, gameFolder, CP_RECURSIVE | CP_NO_TARGET_DIR | CP_LINK);
	if(returnValue < 0) {
		printf("Coping game files.  HINT: having the game on the same partition as you home director will make this operation use zero extra space");
		returnValue = copy(steamGameFolder, gameFolder, CP_RECURSIVE | CP_NO_TARGET_DIR);
		if(returnValue < 0) {
			fprintf(stderr, "Copy failed make sure you have enough space on your device.");
			free(steamGameFolder);
			free(gameFolder);
			return EXIT_FAILURE;
		}
	}
	casefold(gameFolder);

	free(steamGameFolder);
	free(gameFolder);
	printf("Done\n");
	return EXIT_SUCCESS;
}


static int unbind(int argc, char ** argv) {
	if(argc != 3 ) return usage();
	const char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}
	int gameId = getGameIdFromAppId(appid);
	const char * path = g_hash_table_lookup(gamePaths, &gameId);
	char * steamGameFolder = g_build_path("/", path, "steamapps/common", GAMES_NAMES[gameId], "Data", NULL);

	while(umount2(steamGameFolder, MNT_FORCE | MNT_DETACH) == 0);

	free(steamGameFolder);
	return EXIT_SUCCESS;
}

static int removeMod(int argc, char ** argv) {
	if(argc != 4) return usage();
	const char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		fprintf(stderr, "Invalid appid");
		return EXIT_FAILURE;
	}

	//strtoul set EINVAL if the string is invalid
	unsigned long modId = strtoul(argv[3], NULL, 10);
	if(errno == EINVAL) {
		fprintf(stderr, "Modid has to be a valid number\n");
		return EXIT_FAILURE;
	}

	//might crash if no mods were installed
	char * home = getHome();
	char * modFolder = g_build_filename(home, MANAGER_FILES, MOD_FOLDER_NAME, appIdStr, NULL);
	free(home);
	GList * mods = listFilesInFolder(modFolder);
	GList * modsFirstPointer = mods;

	for(unsigned long i = 0; i < modId; i++) {
		mods = g_list_next(mods);
	}

	if(mods == NULL) {
		fprintf(stderr, "Mod not found\n");
		return EXIT_FAILURE;
	}

	gchar * filename = g_build_filename(modFolder, mods->data, NULL);

	delete(filename, true);

	g_free(filename);
	g_free(modFolder);
	g_list_free_full(modsFirstPointer, free);
	return EXIT_SUCCESS;
}

static int fomod(int argc, char ** argv) {
	if(argc != 4) return usage();
	char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
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
	char * home = getHome();
	char * modFolder = g_build_filename(home, MANAGER_FILES, MOD_FOLDER_NAME, appIdStr, NULL);
	free(home);
	GList * mods = listFilesInFolder(modFolder);
	GList * modsFirstPointer = mods;

	for(unsigned long i = 0; i < modId; i++) {
		mods = g_list_next(mods);
	}

	if(mods == NULL) {
		fprintf(stderr, "Mod not found\n");
		return EXIT_FAILURE;
	}

	char * destination = g_strconcat(mods->data, "__FOMOD", NULL);

	if(access(destination, F_OK) == 0) {
		delete(destination, true);
	}
	char * modDestination = g_build_filename(modFolder, destination, NULL);
	char * modPath = g_build_filename(modFolder, mods->data, NULL);

	//TODO: add error handling
	int returnValue = installFOMod(modPath, modDestination);

	free(destination);
	g_list_free_full(modsFirstPointer, free);
	g_free(modDestination);
	g_free(modPath);
	g_free(modFolder);
	return returnValue;
}

static int swapMod(int argc, char ** argv) {
	if(argc != 5) return usage();
	char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		fprintf(stderr, "Invalid appid");
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
	return swapPlace(appid, modIdA, modIdB);
}

int main(int argc, char ** argv) {
	if(argc < 2 ) return usage();

	if(audit_getloginuid() == 0 || isRoot()) {
		fprintf(stderr, "Please don't run this software as root\n");
		return EXIT_FAILURE;
	}

	int searchStatus;
	gamePaths = search_games(&searchStatus);


	int returnValue = EXIT_SUCCESS;
	if(searchStatus == EXIT_FAILURE) {
		returnValue = EXIT_FAILURE;
		fprintf(stderr, "Error while looking up libraries, do you have steam installed ?");
		goto exit;
	}

	char * home = getHome();
	char * configFolder = g_build_filename(home, MANAGER_FILES, NULL);
	free(home);
	if(access(configFolder, F_OK) != 0) {

		//check to NOT run this as root
		//creating folder as root will causes lots of headache
		//such as problem with access rights and taking the risk of
		//running system as root (which is a big security issue)
		if(getuid() == 0) {
			fprintf(stderr, "For the first run please avoid sudo\n");
			returnValue = EXIT_FAILURE;
			goto exit2;
		} else {
			//leading 0 == octal
			int i = g_mkdir_with_parents(configFolder, 0755);
			if(i < 0) {
				fprintf(stderr, "Could not create configs, check access rights for this path: %s", configFolder);
				goto exit2;
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

	else if(strcmp(argv[1], "--deploy") == 0 || strcmp(argv[1], "-d") == 0)
		returnValue = deploy(argc, argv);

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

	else if(strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
		returnValue = EXIT_SUCCESS;
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

exit2:
	g_free(configFolder);
	g_hash_table_destroy(gamePaths);
exit:
	return returnValue;
}
