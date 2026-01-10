#include <optional>
extern "C" {
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/mount.h>
	#include <sys/stat.h>
	#include <sys/wait.h>
	#include <unistd.h>
	#include <glib.h>
	
	#include <errorType.h>
	#include <steam.h>
	#include <constants.h>
	#include "mods.h"
}

#include "overlayfs.hpp"
#include "deploy.hpp"

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::optional<bool> is_deployed(int appid) {
	auto path = get_deploy_target(appid);
	pid_t pid = fork();
	if(pid == -1) {
		g_error("Failed to fork");
	}

	if(pid == 0) {
		execl("/usr/bin/fuser", "fuser", "-M", path.c_str(), NULL);
		exit(EIO);
	} else {
		int value;
		waitpid(pid, &value, 0);
		if(value == EIO) {
			return ERR_FAILURE;
		} else if(value == 0) {
			return TRUE;
		} else {
			return FALSE;
		}
	}

	return std::nullopt;
}

error_t undeploy(int appid) {
	auto appid_str = std::to_string(appid);
	fs::path path = fs::temp_directory_path() / MODLIB_NAME / appid_str;

	pid_t pid = fork();
	if(pid == -1) {
		g_error("Failed to fork");
		return ERR_FAILURE;
	}
	if(pid == 0) {
		//lazy unmount for more reliability
		execl("/usr/bin/fusermount", "fusermount", "-uz", path.c_str(), NULL);
		g_error("failed to run fusermount");
	}

	int value;
	waitpid(pid, &value, 0);
	if(value == 0)
		return ERR_SUCCESS;
	return ERR_FAILURE;
}

static std::vector<std::filesystem::path> list_overlay_dirs(const int appid, fs::path game_folder) {
	auto appid_str = std::to_string(appid);
	fs::path mod_folder = fs::path(g_get_home_dir()) / MODLIB_WORKING_DIR / MOD_FOLDER_NAME / appid_str;

	GList * mods = mods_list(appid);
	//probably over allocating but this doesn't matter that much
	// +2 since we add the game folder
	std::vector<std::filesystem::path> mods_to_install;
	mods_to_install.reserve(g_list_length(mods) + 1);

	GList * iterator = mods;
	for(;iterator != NULL; iterator = g_list_next(iterator)) {
		const char * mod_name = (char *)iterator->data;
		fs::path mod_path = mod_folder / mod_name;

		//if the mod is not marked to be deployed then don't
		if(!fs::exists(mod_path / INSTALLED_FLAG_FILE)) {
			continue;
		}

		//this is made to leave the first case empty so i can put lowerdir in it before feeding it to g_strjoinv
		mods_to_install.push_back(mod_path);
	}

	//const char * data = "lowerdir=gameFolder,gameFolder2,gameFolder3..."
	mods_to_install.push_back(game_folder);

	g_list_free_full(mods, g_free);
	return mods_to_install;
}

fs::path get_deploy_target(int appid) {
	return fs::temp_directory_path() / MODLIB_NAME / std::to_string(appid);
}

DeploymentErrors deploy(int appid) {
	auto appid_str = std::to_string(appid);

	const char * home_path = g_get_home_dir();

	GFile * game_folder_gfile =  steam_get_game_folder_path(appid);
	g_autofree char * game_folder_str = g_file_get_path(game_folder_gfile);
	fs::path game_folder = game_folder_str;

	if(game_folder_gfile == NULL) {
		return DeploymentErrors::GAME_NOT_FOUND;
	}

	if(!g_file_query_exists(game_folder_gfile, NULL)) {
		return DeploymentErrors::GAME_NOT_FOUND;
	}

	auto overlay_dirs = list_overlay_dirs(appid, game_folder);
	fs::path game_upper_dir = fs::path(home_path) / MODLIB_WORKING_DIR / GAME_UPPER_DIR_NAME / appid_str.c_str();

	if(!fs::exists(game_upper_dir))
		fs::create_directories(game_upper_dir);

	//discard the nodiscard as an unmounted fs will return an error, and in our case it's fine
	(void) undeploy(appid);
	auto mount = fs::temp_directory_path() / MODLIB_NAME / appid_str;
	fs::create_directories(mount);

	OverlayErrors status = overlay_mount(overlay_dirs, mount, game_upper_dir);

	switch(status) {
	case OverlayErrors::SUCCESS:
		return DeploymentErrors::OK;
	case OverlayErrors::FAILURE:
		g_warning("Mount failure");
		return DeploymentErrors::CANNOT_MOUNT;
	case OverlayErrors::NOT_INSTALLED:
		g_warning("Modfs is missing");
		return DeploymentErrors::MODFS_NOT_INSTALLED;
	default:
		return DeploymentErrors::BUG;
	}
}

//TO remove once meson has good rust crates support
extern "C" {
	cDeploymentErrors_t _deploy(int appid) {
		return (cDeploymentErrors_t)deploy(appid);
	}

	error_t _undeploy(int appid) {
		return (error_t)undeploy(appid);
	}
	
	error_t _is_deployed(int appid, bool * status) {
		auto deploy = is_deployed(appid);
		if(!deploy) {
			return ERR_FAILURE;
		}
		*status = *deploy;
		return ERR_SUCCESS;
	};

	char * _get_deploy_target(int appid) {
		auto target = get_deploy_target(appid);
		return strdup(target.c_str());
	};
}
