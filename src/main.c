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

GHashTable * gamePaths;

bool isRoot() {
	return getuid() == 0;
}

GList * listFilesInFolder(const char * path) {
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

int noRoot() {
	printf("Don't run this argument as root\n");
	return EXIT_FAILURE;
}

int needRoot() {
	printf("Root is needed to bind with the game files\n");
	return EXIT_FAILURE;
}

int usage() {
	printf("Use --list-games or -l to list compatible games\n");
	printf("Use --add or -a <APPID> <FILENAME> to add a mod to a game\n");
	printf("Use --list-mods or -m <APPID> to list all mods for a game\n");
	printf("Use --install or -i <APPID> <MODID> to add a mod to a game\n");
	printf("Use --remove or -r <APPID> <MODID> to remove a mod from a game\n");
	printf("Use --deploy or -d <APPID> to deploy the mods for the game\n");
	printf("Use --unbind or -u <APPID> to rollback a deployment\n");
	//TODO: as bonus
	//printf("Use --start-game <APPID> to deploy the mods and launch the game through steam\n");
	//printf("Use --steam in team cmdline options to deploy directly on game startup\n");
	return EXIT_FAILURE;
}

int validateAppId(const char * appIdStr) {
	//strtoul set EINVAL(after C99) if the string is invalid
	u_int32_t appid = strtoul(appIdStr, NULL, 10);
	if(errno == EINVAL) {
		printf("Appid has to be a valid number\n");
		return -1;
	}

	int gameId = getGameIdFromAppId(appid);
	if(gameId < 0) {
		printf("Game is not compatible\n");
		return -1;
	}

	if(!g_hash_table_contains(gamePaths, &gameId)) {
		printf("Game not found\n");
		return -1;
	}

	return appid;
}

int listGames(int argc, char ** argv) {
	if(argc != 2) return usage();
	GList * gamesIds = g_hash_table_get_keys(gamePaths);
	if(g_list_length(gamesIds) == 0) {
		printf("No game found\n");
	} else {
		while(gamesIds != NULL) {
			int * gameIndex = (int*)gamesIds->data;
			printf("%d: %s\n", GAMES_APPIDS[*gameIndex], GAMES_NAMES[*gameIndex]);
			gamesIds = g_list_next(gamesIds);
		}
	}
	return EXIT_SUCCESS;
}

int add(int argc, char ** argv) {
	if(argc != 4) return usage();
		char * appIdStr = argv[2];

	u_int32_t appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	addMod(argv[3], appid);
	return EXIT_SUCCESS;
}

int listMods(int argc, char ** argv) {
	if(argc != 3) return usage();
	char * appIdStr = argv[2];
	if(validateAppId(appIdStr) < 0) {
		return EXIT_FAILURE;
	}

	//might crash if no mods were installed
	char * modFolder = g_build_filename(getenv("HOME"), MANAGER_FILES, MOD_FOLDER_NAME, appIdStr, NULL);
	GList * mods = listFilesInFolder(modFolder);
	unsigned short index = 0;

	printf("Id | Installed  | Name\n");
	while(mods != NULL) {
		char * modName = (char*)mods->data;
		char * modPath = g_build_filename(modFolder, modName, INSTALLED_FLAG_FILE, NULL);

		if(access(modPath, F_OK) == 0) {
			printf("\033[0;32m %d | ✓ | %s\n", index, modName);
		} else {
			printf("\033[0;31m %d | ✕ | %s\n", index, modName);
		}

		index++;
		mods = g_list_next(mods);
	}


	free(modFolder);
	free(mods);
	return EXIT_SUCCESS;
}


/*
 * THIS WILL NOT INSTALL THE MOD
 * this will just flag the mod as needed to be deployed
 * it's the deploying process that handle the rest
*/
int installAndRemoveMod(int argc, char ** argv, bool install) {
	if(argc != 4) return usage();
	char * appIdStr = argv[2];
	u_int32_t appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	//strtoul set EINVAL(after C99) if the string is invalid
	u_int32_t modId = strtoul(argv[3], NULL, 10);
	if(errno == EINVAL) {
		printf("Appid has to be a valid number\n");
		return -1;
	}

	//might crash if no mods were installed
	char * modFolder = g_build_filename(getenv("HOME"), MANAGER_FILES, MOD_FOLDER_NAME, appIdStr, NULL);
	GList * mods = listFilesInFolder(modFolder);
	GList * modsFirstPointer = mods;

	for(int i = 0; i < modId; i++) {
		mods = g_list_next(mods);
	}

	if(mods == NULL) {
		printf("Mod not found\n");
		return EXIT_FAILURE;
	}

	char * modName = (char*)mods->data;
	char * modFlag = g_build_filename(modFolder, modName, INSTALLED_FLAG_FILE, NULL);

	if(install) {
		if(access(modFlag, F_OK) == 0) {
			printf("The mod is already activated \n");
			return EXIT_SUCCESS;
		}

		//Create activated file
		FILE * fd = fopen(modFlag, "w");
		fwrite("", 1, 1, fd);
		fclose(fd);
	} else {
		if(access(modFlag, F_OK) != 0) {
			printf("The mod is not activated \n");
			return EXIT_SUCCESS;
		}

		if(unlink(modFlag) != 0) {
			printf("Error could not disable the mod.\n");
			printf("please remove this file '%s' as root\n", modFlag);
			return EXIT_FAILURE;
		}
	}

	free(modFolder);
	g_list_free(modsFirstPointer);
	return EXIT_SUCCESS;
}

int deploy(int argc, char ** argv) {
	if(argc != 3) return usage();

	char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}

	char * home = getHome();
	char * gameFolder = g_build_filename(home, MANAGER_FILES, GAME_FOLDER_NAME, appIdStr, NULL);

	if(access(gameFolder, F_OK) != 0) {
		printf("Please run '%s --setup %d' first", APP_NAME, appid);
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

	//const char * data = "lowerdir=gameFolder,gameFolder2,gameFolder3..."
	modsToInstall[modCount] = gameFolder;
	modsToInstall[modCount + 1] = NULL;
	printf("Mounting the overlay\n");
	int gameId = getGameIdFromAppId(appid);
	const char * path = g_hash_table_lookup(gamePaths, &gameId);
	char * steamGameFolder = g_build_path("/", path, "steamapps/common", GAMES_NAMES[gameId], "Data", NULL);
	printf("%s\n", steamGameFolder);

	char * gameUpperDir = g_build_filename(home, MANAGER_FILES, GAME_UPPER_DIR_NAME, appIdStr, NULL);
	char * gameWorkDir = g_build_filename(home, MANAGER_FILES, GAME_WORK_DIR_NAME, appIdStr, NULL);

	//unmount the game folder
	//DETACH + FORCE allow us to be sure it will be unload.
	//it might crash / corrupt game file if the user do it while the game is running
	//but it's still very unlikely
	while(umount2(steamGameFolder, MNT_FORCE | MNT_DETACH) == 0);
	int status = overlayMount(modsToInstall, steamGameFolder, gameUpperDir, gameWorkDir);
	if(status == 0) {
		printf("Everything is ready, just launch the game\n");
	} else if(status == -1) {
		printf("Could not mount the mods overlay, try to install fuse-overlay\n");
		return EXIT_FAILURE;
	} else if(status == -2) {
		printf("Could not mount the mods overlay\n");
		return EXIT_FAILURE;
	} else {
		printf("%d take a screenshot and go insult the dev that let this passthrough\n", __LINE__);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int setup(int argc, char ** argv) {
	if(argc != 3 ) return usage();
	char * appIdStr = argv[2];
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
	const int returnValue = copy(steamGameFolder, gameFolder, CP_RECURSIVE | CP_NO_TARGET_DIR | CP_LINK);
	if(returnValue < 0) copy(steamGameFolder, gameFolder, CP_RECURSIVE | CP_NO_TARGET_DIR);

	free(steamGameFolder);
	free(gameFolder);
	printf("Done\n");
	return EXIT_SUCCESS;
}

int unbind(int argc, char ** argv) {
	if(argc != 3 ) return usage();
	char * appIdStr = argv[2];
	int appid = validateAppId(appIdStr);
	if(appid < 0) {
		return EXIT_FAILURE;
	}
	int gameId = getGameIdFromAppId(appid);
	const char * path = g_hash_table_lookup(gamePaths, &gameId);
	char * steamGameFolder = g_build_path("/", path, "steamapps/common", GAMES_NAMES[gameId], "Data", NULL);
	while(umount2(steamGameFolder, MNT_FORCE | MNT_DETACH));

	free(steamGameFolder);
	return EXIT_SUCCESS;
}

int main(int argc, char ** argv) {
	if(argc < 2 ) return usage();


	if(audit_getloginuid() == 0) {
		printf("Root user should not be using this\n");
		return EXIT_FAILURE;
	}

	gamePaths = search_games();




	char * configFolder = g_build_filename(getHome(), MANAGER_FILES, NULL);
	if(access(configFolder, F_OK) != 0) {
		//check to NOT run this as root
		//creating folder as root will causes lots of headache
		if(getuid() == 0) {
			printf("For the first please run without sudo\n");
			return EXIT_FAILURE;
		} else {
			//leading 0 == octal
			int i = g_mkdir_with_parents(configFolder, 0755);
			if(i < 0) {
				printf("Could not create configs, check access rights for this path: %s", configFolder);
			}
			//try to enable casefold
			//failure would have no impact
			char * chattrcommand = g_strjoin("", "chattr +F ",configFolder, " 2> /dev/null", NULL);
			system(chattrcommand);
			free(chattrcommand);
		}
	}
	free(configFolder);


	if(strcmp(argv[1], "--list-games") == 0 || strcmp(argv[1], "-l") == 0) {
		if(isRoot()) return noRoot();
		return listGames(argc, argv);
	} else if(strcmp(argv[1], "--add") == 0 || strcmp(argv[1], "-a") == 0) {
		if(isRoot()) return noRoot();
		return add(argc, argv);
	} else if(strcmp(argv[1], "--list-mods") == 0 || strcmp(argv[1], "-m") == 0) {
		if(isRoot()) return noRoot();
		return listMods(argc, argv);
	} else if(strcmp(argv[1], "--install") == 0 || strcmp(argv[1], "-i") == 0) {
		if(isRoot()) return noRoot();
		return installAndRemoveMod(argc, argv, true);
	} else if(strcmp(argv[1], "--remove") == 0 || strcmp(argv[1], "-r") == 0) {
		if(isRoot()) return noRoot();
		return installAndRemoveMod(argc, argv, false);
	} else if(strcmp(argv[1], "--deploy") == 0 || strcmp(argv[1], "-d") == 0) {
		if(!isRoot()) return needRoot();
		return deploy(argc, argv);
	} else if(strcmp(argv[1], "--unbind") == 0 || strcmp(argv[1], "-u") == 0){
		if(!isRoot()) return needRoot();
		return unbind(argc, argv);
	} else if(strcmp(argv[1], "--setup") == 0) {
		if(isRoot()) return noRoot();
		return setup(argc, argv);
	} else {
		return usage();
	}


	return 0;
}