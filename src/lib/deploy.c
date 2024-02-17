#include <stdlib.h>
#include <sys/mount.h>
#include <unistd.h>

#include <errorType.h>
#include <deploy.h>
#include <steam.h>
#include <constants.h>

#include "gameData.h"
#include "file.h"
#include "mods.h"
#include "overlayfs.h"



DeploymentErrors_t deploy(char * appid_str, GList ** ignored_mods) {
	int appid = steam_parseAppId(appid_str);
	if(appid < 0) {
		return INVALID_APPID;
	}

	const char * home_path = g_get_home_dir();
	g_autofree char * game_folder = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_FOLDER_NAME, appid_str, NULL);

	//is the game present in the disk
	if(access(game_folder, F_OK) != 0) {
		return GAME_NOT_FOUND;
	}

	GFile * data_folder_file = NULL;
	error_t error = game_data_get_data_path(appid, &data_folder_file);
	if(error != ERR_SUCCESS) {
		return GAME_NOT_FOUND;
	}
	char * data_folder = g_file_get_path(data_folder_file);

	//unmount everything beforhand
	//might crash if no mods were installed
	g_autofree char * mod_folder = g_build_filename(home_path, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, NULL);

	GList * mods = mods_list(appid);
	//since the priority is by alphabetical order
	//and we need to bind the least important first
	//and the most important last
	//we have to reverse the list
	mods = g_list_reverse(mods);

	//probably over allocating but this doesn't matter that much
	char * mods_to_install[g_list_length(mods) + 2];
	int mod_count = 0;

	while(true) {
		if(mods == NULL)break;
		char * mod_name = (char *)mods->data;
		char * mod_path = g_build_path("/", mod_folder, mod_name, NULL);
		char * mod_flag = g_build_filename(mod_path, INSTALLED_FLAG_FILE, NULL);

		//if the mod is not marked to be deployed then don't
		if(access(mod_flag, F_OK) != 0) {
			*ignored_mods = g_list_append(*ignored_mods, mod_name);
			mods = g_list_next(mods);
			continue;
		}

		//this is made to leave the first case empty so i can put lowerdir in it before feeding it to g_strjoinv
		mods_to_install[mod_count] = mod_path;
		mod_count += 1;

		mods = g_list_next(mods);
		g_free(mod_flag);
	}

	//const char * data = "lowerdir=gameFolder,gameFolder2,gameFolder3..."
	mods_to_install[mod_count] = game_folder;
	mods_to_install[mod_count + 1] = NULL;

	g_autofree char * game_upper_dir = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_UPPER_DIR_NAME, appid_str, NULL);
	g_autofree char * game_work_dir = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_WORK_DIR_NAME, appid_str, NULL);


	//unmount the game folder
	//DETACH + FORCE allow us to be sure it will be unload.
	//it might crash / corrupt game file if the user do it while the game is running
	//but it's still very unlikely
	while(umount2(data_folder, MNT_FORCE | MNT_DETACH) == 0);
	overlay_errors_t status = overlay_mount(mods_to_install, data_folder, game_upper_dir, game_work_dir);
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
