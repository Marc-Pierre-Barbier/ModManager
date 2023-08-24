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



deploymentErrors_t deploy(char * appIdStr, GList ** ignoredMods) {
	int appid = steam_parseAppId(appIdStr);
	if(appid < 0) {
		return INVALID_APPID;
	}

	g_autofree GFile * home = audit_get_home();
	g_autofree char * home_path = g_file_get_path(home);
	g_autofree char * gameFolder = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_FOLDER_NAME, appIdStr, NULL);

    //is the game present in the disk
	if(access(gameFolder, F_OK) != 0) {
		return GAME_NOT_FOUND;
	}

    GFile * data_folder_file = NULL;
	error_t error = gameData_getDataPath(appid, &data_folder_file);
	if(error != ERR_SUCCESS) {
		return GAME_NOT_FOUND;
	}
	char * data_folder = g_file_get_path(data_folder_file);

	//unmount everything beforhand
	//might crash if no mods were installed
	g_autofree char * modFolder = g_build_filename(home_path, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appIdStr, NULL);

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
		g_free(modFlag);
	}

	//const char * data = "lowerdir=gameFolder,gameFolder2,gameFolder3..."
	modsToInstall[modCount] = gameFolder;
	modsToInstall[modCount + 1] = NULL;

	g_autofree char * gameUpperDir = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_UPPER_DIR_NAME, appIdStr, NULL);
	g_autofree char * gameWorkDir = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_WORK_DIR_NAME, appIdStr, NULL);


	//unmount the game folder
	//DETACH + FORCE allow us to be sure it will be unload.
	//it might crash / corrupt game file if the user do it while the game is running
	//but it's still very unlikely
	while(umount2(data_folder, MNT_FORCE | MNT_DETACH) == 0);
	overlay_errors_t status = overlay_mount(modsToInstall, data_folder, gameUpperDir, gameWorkDir);
	if(status == SUCESS) {
		return OK;
	} else if(status == FAILURE) {
		return CANNOT_MOUNT;
	} else if(status == NOT_INSTALLED) {
		return FUSE_NOT_INSTALLED;
	} else {
		return BUG;
	}

	g_free(data_folder);
	return EXIT_SUCCESS;
}
