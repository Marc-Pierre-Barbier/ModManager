extern "C" {
	#include "file.h"
	#include "constants.h"
	#include "mods.h"
	#include "steam.h"
}

#include <algorithm>
#include <vector>
#include <sys/stat.h>

#include <fomod.hpp>
#include "fomodTypes.hpp"


static bool priority_cmp(const FOModFile &a, const FOModFile &b) {
	return a.priority < b.priority;
}

void fomod_execute_file_operations(std::vector<FOModFile> &pending_file_operations, const int mod_id, const int appid) {
	char app_id_str[GAMES_MAX_APPID_LENGTH];
	snprintf(app_id_str, GAMES_MAX_APPID_LENGTH, "%d", appid);
	const int gameId = steam_game_id_from_app_id(appid);

	GList * mods = mods_list(appid);
	const char * souce_name = reinterpret_cast<char *>(g_list_nth(mods, mod_id)->data);
	const char * home = g_get_home_dir();
	g_autofree const char * mods_folder = g_build_filename(home, MODLIB_WORKING_DIR, MOD_FOLDER_NAME, app_id_str, NULL);
	g_autofree  GFile * mod_folder = g_file_new_build_filename(mods_folder, souce_name, NULL);
	g_autofree const char * destination_name = g_strconcat(souce_name, "__FOMOD", NULL);
	g_autofree char * target = g_ascii_strdown(GAMES_MOD_TARGET[gameId], -1);
	g_autofree const char * destination_folder = g_build_filename(mods_folder, destination_name, target, NULL);

	//priority higher a less important and should be processed first.
	std::sort(pending_file_operations.begin(), pending_file_operations.end(), priority_cmp);

	g_autofree char * mod_folder_path = g_file_get_path(mod_folder);
	mkdir(destination_folder, 0700);

	for(auto &file : pending_file_operations) {
		//TODO: support destination
		//no using link since priority is made to override files and link is annoying to deal with when overriding files.

		////fix the / and \ windows - unix paths
		//xml_fix_path(file->source);

		//TODO: check if it can build from 2 path
		g_autofree GFile * source = g_file_new_build_filename(mod_folder_path, file.source.c_str(), NULL);

		error_t copy_result = ERR_SUCCESS;
		if(file.isFolder) {
			g_autofree GFile * destination = g_file_new_build_filename(destination_folder, file.destination.c_str(), NULL);
			g_autofree char * destination_str = g_file_get_path(destination);
			g_mkdir_with_parents(destination_str, 0700);
			copy_result = file_recursive_copy(source, destination, G_FILE_COPY_NONE, NULL, NULL);
		} else {
			GError * err = NULL;
			//basename is unsafe
			g_autofree char * file_destination = strdup(file.destination.c_str());
			g_autofree GFile * destination = g_file_new_build_filename(destination_folder, basename(file_destination), NULL);
			if(!g_file_copy(source, destination, G_FILE_COPY_NONE, NULL, NULL, NULL, &err)) {
				fprintf(stderr, "%s\n", err->message);
				copy_result = ERR_FAILURE;
			}
		}
		if(copy_result != ERR_SUCCESS) {
			g_warning( "Copy failed, some file might be corrupted\n");
		}

	}

	g_list_free_full(mods, g_free);
}

void fomod_process_cond_files(const FOMod &fomod, const std::vector<FOModFlag> &flag_list, std::vector<FOModFile> &pending_file_operations) {
	for(const auto &cond_file : fomod.cond_files) {
		bool are_all_flags_valid = true;

		//checking if all flags are valid
		for(auto &flag : cond_file.required_flags) {
			//not sure of this implementation
			auto it = std::ranges::find_if(flag_list.begin(), flag_list.end(), [flag](const FOModFlag &a){
				return a.name == flag.name && a.name == flag.name;
			});

			if(it != flag_list.end()) {
				are_all_flags_valid = false;
				break;
			}
		}

		if(are_all_flags_valid) {
			for(auto &file : cond_file.files) {
				pending_file_operations.push_back(file);
			}
		}
	}
}
