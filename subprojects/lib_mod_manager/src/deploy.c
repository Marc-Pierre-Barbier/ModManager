#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>

#include <errorType.h>
#include <deploy.h>
#include <steam.h>
#include <constants.h>

#include "mods.h"
#include "overlayfs.h"

error_t is_deployed(int appid, bool * status) {
	g_autofree char * path = get_deploy_target(appid);
	pid_t pid = fork();
	if(pid == -1) {
		g_error("Failed to fork");
	}

	if(pid == 0) {
		execl("/usr/bin/fuser", "fuser", "-M", path, NULL);
		exit(EIO);
	} else {
		int value;
		waitpid(pid, &value, 0);
		if(value == EIO) {
			return ERR_FAILURE;
		} else if(value == 0) {
			*status = TRUE;
		} else {
			*status = FALSE;
		}
	}

	return ERR_SUCCESS;
}

error_t undeploy(int appid) {
	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	g_autofree char * path = g_build_filename(g_get_tmp_dir(), MODLIB_NAME, appid_str, NULL);

	pid_t pid = fork();
	if(pid == -1) {
		g_error("Failed to fork");
		return ERR_FAILURE;
	}
	if(pid == 0) {
		//lazy unmount for more reliability
		execl("/usr/bin/fusermount", "fusermount", "-uz", path, NULL);
		g_error("failed to run fusermount");
	}

	int value;
	waitpid(pid, &value, 0);
	if(value == 0)
		return ERR_SUCCESS;
	return ERR_FAILURE;
}

static char ** list_overlay_dirs(const int appid, const char * game_folder) {
	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	g_autofree char * mod_folder = g_build_filename(g_get_home_dir(), MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, NULL);

	GList * mods = mods_list(appid);
	//probably over allocating but this doesn't matter that much
	// +2 since we add the game folder
	char ** mods_to_install = malloc(sizeof(char *) * (g_list_length(mods) + 2));
	int mod_count = 0;

	GList * iterator = mods;
	for(;iterator != NULL; iterator = g_list_next(iterator)) {
		const char * mod_name = (char *)iterator->data;
		char * mod_path = g_build_path("/", mod_folder, mod_name, NULL);
		g_autofree char * mod_flag = g_build_filename(mod_path, INSTALLED_FLAG_FILE, NULL);

		//if the mod is not marked to be deployed then don't
		if(access(mod_flag, F_OK) != 0) {
			g_free(mod_path);
			continue;
		}

		//this is made to leave the first case empty so i can put lowerdir in it before feeding it to g_strjoinv
		mods_to_install[mod_count] = mod_path;
		mod_count += 1;
	}

	//const char * data = "lowerdir=gameFolder,gameFolder2,gameFolder3..."
	mods_to_install[mod_count] = strdup(game_folder);
	mods_to_install[mod_count + 1] = NULL;

	g_list_free_full(mods, g_free);
	return mods_to_install;
}

char * get_deploy_target(int appid) {
	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	return g_build_filename(g_get_tmp_dir(), MODLIB_NAME, appid_str, NULL);
}

DeploymentErrors_t deploy(int appid) {
	char appid_str[GAMES_MAX_APPID_LENGTH];
	snprintf(appid_str, GAMES_MAX_APPID_LENGTH, "%d", appid);

	const char * home_path = g_get_home_dir();

	GFile * game_folder_gfile =  steam_get_game_folder_path(appid);
	if(game_folder_gfile == NULL) {
		return GAME_NOT_FOUND;
	}

	if(!g_file_query_exists(game_folder_gfile, NULL)) {
		return GAME_NOT_FOUND;
	}

	g_autofree GFile * dest = g_file_new_build_filename(g_get_tmp_dir(), appid_str, NULL); //TODO: put it in a subfolder

	g_autofree char * game_folder = g_file_get_path(game_folder_gfile);
	char ** overlay_dirs = list_overlay_dirs(appid, game_folder);

	g_autofree char * game_upper_dir = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_UPPER_DIR_NAME, appid_str, NULL);
	if(access(game_upper_dir, F_OK) != 0)
		g_mkdir_with_parents(game_upper_dir, 0777);

	//discard the nodiscard as an unmounted fs will return an error, and in our case it's fine
	(void) undeploy(appid);
	g_autofree char * mount = g_build_filename(g_get_tmp_dir(), MODLIB_NAME, appid_str, NULL);
	g_mkdir_with_parents(mount, 0777);
	overlay_errors_t status = overlay_mount(overlay_dirs, mount, game_upper_dir);

	char ** overlay_dirs_it = overlay_dirs;
	while((*overlay_dirs_it) != NULL) {
		g_free(*overlay_dirs_it);
		overlay_dirs_it++;
	}
	g_free(overlay_dirs);

	if(status == SUCCESS) {
		return OK;
	} else if(status == FAILURE) {
		g_warning("Mount failure");
		return CANNOT_MOUNT;
	} else if(status == NOT_INSTALLED) {
		g_warning("Modfs is missing");
		return MODFS_NOT_INSTALLED;
	} else {
		return BUG;
	}
}
