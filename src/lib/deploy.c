#include <stdlib.h>
#include <sys/mount.h>
#include <unistd.h>

#include <errorType.h>
#include <deploy.h>
#include <steam.h>
#include <constants.h>

#include "gameData.h"
#include "getHome.h"
#include "file.h"
#include "mods.h"
#include "overlayfs.h"
#include "install.h"



deploymentErrors_t deploy(char * appIdStr, GList ** ignoredMods) {
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		return INVALID_APPID;
	}

	char * home = getHome();
	char * gameFolder = g_build_filename(home, MODLIB_WORKING_DIR, GAME_FOLDER_NAME, appIdStr, NULL);

    //is the game present in the disk
	if(access(gameFolder, F_OK) != 0) {
		free(home);
		g_free(gameFolder);
		return GAME_NOT_FOUND;
	}

    char * dataFolder = NULL;
	error_t error = gameData_getDataPath(appid, &dataFolder);
	if(error != ERR_SUCCESS) {
		free(home);
		return GAME_NOT_FOUND;
	}

	//unmount everything beforhand
	//might crash if no mods were installed
	char * modFolder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appIdStr, NULL);

	GList * mods = mods_list(appid);
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
			*ignoredMods = g_list_append(*ignoredMods, modName);
			mods = g_list_next(mods);
			continue;
		}

		//this is made to leave the first case empty so i can put lowerdir in it before feeding it to g_strjoinv
		modsToInstall[modCount] = modPath;
		modCount += 1;

		mods = g_list_next(mods);
		free(modFlag);
	}

	g_free(modFolder);

	//const char * data = "lowerdir=gameFolder,gameFolder2,gameFolder3..."
	modsToInstall[modCount] = gameFolder;
	modsToInstall[modCount + 1] = NULL;

	char * gameUpperDir = g_build_filename(home, MODLIB_WORKING_DIR, GAME_UPPER_DIR_NAME, appIdStr, NULL);
	char * gameWorkDir = g_build_filename(home, MODLIB_WORKING_DIR, GAME_WORK_DIR_NAME, appIdStr, NULL);
	free(home);

	//unmount the game folder
	//DETACH + FORCE allow us to be sure it will be unload.
	//it might crash / corrupt game file if the user do it while the game is running
	//but it's still very unlikely
	while(umount2(dataFolder, MNT_FORCE | MNT_DETACH) == 0);
	overlay_errors_t status = overlay_mount(modsToInstall, dataFolder, gameUpperDir, gameWorkDir);
	if(status == SUCESS) {
		return OK;
	} else if(status == FAILURE) {
		return CANNOT_MOUNT;
	} else if(status == NOT_INSTALLED) {
		return FUSE_NOT_INSTALLED;
	} else {
		return BUG;
	}

	g_free(dataFolder);
	g_free(gameUpperDir);
	g_free(gameWorkDir);

	return EXIT_SUCCESS;
}
