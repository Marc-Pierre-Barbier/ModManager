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


static error_t generate_symlink_copy(const char * source, const char * destination) {
	//generate a folder where all files contained within are symlinks and folders are just normal folders.
	//this allow me to make everything lowercase so overlay fs can manage collisions

	//is the game accecible
	if(access(source, F_OK) != 0) {
		return ERR_FAILURE;
	}

	if(access(destination, F_OK) != 0) {
		if(mkdir(destination, 0777) != 0)
			return ERR_FAILURE;
	}

	//the actual copy
	DIR * d = opendir(source);
	struct dirent *dir_ent;
	if (d != NULL) {
		while ((dir_ent = readdir(d)) != NULL) {
			if (strcmp(dir_ent->d_name, ".") == 0 || strcmp(dir_ent->d_name, "..") == 0) {
				continue;
			}
			g_autofree char * filename_low = g_ascii_strdown(dir_ent->d_name, -1);
			g_autofree char * from = g_build_filename(source, dir_ent->d_name, NULL);
			g_autofree char * to = g_build_filename(destination, filename_low, NULL);
			g_autofree GFile * from_f = g_file_new_for_path(from);
			g_autofree GFile * to_f = g_file_new_for_path(to);

			//TODO: error handling
			switch(dir_ent->d_type) {
			case DT_LNK:
				g_file_copy(from_f, to_f, G_FILE_COPY_ALL_METADATA, NULL, NULL, NULL, NULL);
				break;
			case DT_DIR:
				generate_symlink_copy(from, to);
				break;
			default:
				symlink(from, to);
				break;
			}
		}
		closedir(d);
	}
	return ERR_SUCCESS;
}

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
	char appid_str[snprintf(NULL, 0, "%d", appid)];
	sprintf(appid_str, "%d", appid);

	g_autofree char * path = g_build_filename(g_get_tmp_dir(), MODLIB_NAME, appid_str, NULL);

	pid_t pid = fork();
	if(pid == -1) {
		g_error("Failed to fork");
		return ERR_FAILURE;
	}
	if(pid == 0) {
		execl("/usr/bin/fusermount", "fusermount", "-u", path, NULL);
		g_error("failed to run fusermount");
	}

	int value;
	waitpid(pid, &value, 0);
	if(value == 0)
		return ERR_SUCCESS;
	return ERR_FAILURE;
}

static char ** list_overlay_dirs(const int appid, const char * game_folder) {
	char appid_str[snprintf(NULL, 0, "%d", appid)];
	sprintf(appid_str, "%d", appid);

	g_autofree char * mod_folder = g_build_filename(g_get_home_dir(), MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, NULL);

	GList * mods = mods_list(appid);
	//since the priority is by alphabetical order
	//and we need to bind the least important first
	//and the most important last
	//we have to reverse the list
	GList * mods_reverse_it = g_list_reverse(mods);

	//probably over allocating but this doesn't matter that much
	// +2 since we add the game folder
	char ** mods_to_install = malloc(g_list_length(mods) + 1);
	int mod_count = 0;

	for(;mods_reverse_it != NULL; mods_reverse_it = g_list_next(mods)) {
		const char * mod_name = (char *)mods_reverse_it->data;
		char * mod_path = g_build_path("/", mod_folder, mod_name, NULL);
		g_autofree char * mod_flag = g_build_filename(mod_path, INSTALLED_FLAG_FILE, NULL);

		//if the mod is not marked to be deployed then don't
		if(access(mod_flag, F_OK) != 0) {
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
	char appid_str[snprintf(NULL, 0, "%d", appid)];
	sprintf(appid_str, "%d", appid);

	return g_build_filename(g_get_tmp_dir(), MODLIB_NAME, appid_str, NULL);
}

DeploymentErrors_t deploy(int appid) {
	char appid_str[snprintf(NULL, 0, "%d", appid)];
	sprintf(appid_str, "%d", appid);

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
	g_autofree char * destination = g_file_get_path(dest);
	if(!g_file_query_exists(dest, NULL)) { //TODO: check if file changes happened in the source (IE: handle updates and manual installs)
		//it's a direct X is needed to open it
		g_mkdir_with_parents(destination, 0777);
		error_t err = generate_symlink_copy(game_folder, destination);
		if(err != ERR_SUCCESS) {
			return CANNOT_SYM_COPY;
		}
	}

	char ** overlay_dirs = list_overlay_dirs(appid, destination);

	g_autofree char * game_upper_dir = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_UPPER_DIR_NAME, appid_str, NULL);
	g_autofree char * game_work_dir = g_build_filename(home_path, MODLIB_WORKING_DIR, GAME_WORK_DIR_NAME, appid_str, NULL);
	if(access(game_upper_dir, F_OK) != 0)
		g_mkdir_with_parents(game_upper_dir, 0777);
	if(access(game_work_dir, F_OK) != 0)
		g_mkdir_with_parents(game_work_dir, 0777);

	//discard the nodiscard as an unmounted fs will return an error, and in our case it's fine
	(void) undeploy(appid);
	g_autofree char * mount = g_build_filename(g_get_tmp_dir(), MODLIB_NAME, appid_str, NULL);
	g_mkdir_with_parents(mount, 0777);
	overlay_errors_t status = overlay_mount(overlay_dirs, mount, game_upper_dir, game_work_dir);

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
		g_warning("Fuse is missing");
		return FUSE_NOT_INSTALLED;
	} else {
		return BUG;
	}
}
