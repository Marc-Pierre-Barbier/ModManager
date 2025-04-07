#include <linux/limits.h>
#include <errorType.h>
#include <gio/gio.h>
#include <glib.h>
#include <game_executables.h>
#include <stdio.h>
#include <steam.h>
#include <constants.h>
#include <string.h>
#include <time.h>
#include <deploy.h>
#include <mods.h>
#include <sys/stat.h>

static error_t find_all_executables(const char * directory, GList ** output) {
	struct dirent * dirent;
	DIR * dir = opendir(directory);
	if(dir == NULL)
		return ERR_FAILURE;
	while ((dirent = readdir(dir))) {
		g_autofree const char * filepath = g_build_filename(directory, dirent->d_name, NULL);
		if(S_ISDIR(dirent->d_type)) {
			error_t error = find_all_executables(filepath, output);
			if(error == ERR_FAILURE)
				return error;
			continue;
		} else if (!S_ISREG(dirent->d_type)) {
			continue;
		}

		const int len = strlen(dirent->d_name);
		if(len < 4) {
			continue;
		}

		const char * file_extention = &dirent->d_name[len - 4];
		if(strcmp(file_extention, ".exe") == 0) {
			*output = g_list_append(*output, strdup(filepath));
		}
	}

	return ERR_SUCCESS;
}

error_t list_game_executables(int appid, GList ** executables) {
	if(executables == NULL)
		return EXIT_FAILURE;

	char appid_str[snprintf(NULL, 0, "%d", appid)];
	sprintf(appid_str, "%d", appid);
	g_autofree char * mod_folder = g_build_filename(g_get_home_dir(), MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, NULL);
	GList * mods = mods_list(appid);
	GList * mods_it = mods;

	for(;mods_it != NULL; mods_it = g_list_next(mods)) {
		const char * mod_name = (char *)mods_it->data;
		g_autofree char * mod_path = g_build_path("/", mod_folder, mod_name, NULL);
		g_autofree char * mod_flag = g_build_filename(mod_path, INSTALLED_FLAG_FILE, NULL);

		//if the mod is not marked to be deployed then skip
		if(access(mod_flag, F_OK) != 0) {
			continue;
		}

		//find all executable
		error_t err = find_all_executables(mod_path, executables);
		if(err == ERR_FAILURE) {
			return ERR_FAILURE;
		}
	}
	return ERR_SUCCESS;
}

error_t set_game_executable(int appid, const char * executable) {
	if(executable == NULL || executable[0] != '/')
		return ERR_FAILURE;

	char appid_str[snprintf(NULL, 0, "%d", appid)];
	sprintf(appid_str, "%d", appid);
	g_autofree char * executable_flag_file = g_build_filename(g_get_home_dir(), MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, GAME_EXECUTABLE_FLAG_FILE, NULL);
	size_t length = strlen(executable_flag_file);

	FILE * file = fopen(executable_flag_file, "w");
	long written = fwrite(executable_flag_file, sizeof(char), length, file);
	fclose(file);

	if(written == length) {
		return ERR_SUCCESS;
	}
	return ERR_FAILURE;
}

char * get_game_executable(int appid) {
	char appid_str[snprintf(NULL, 0, "%d", appid)];
	sprintf(appid_str, "%d", appid);
	g_autofree GFile * executable_flag_file = g_file_new_build_filename(g_get_home_dir(), MODLIB_WORKING_DIR, MOD_FOLDER_NAME, appid_str, GAME_EXECUTABLE_FLAG_FILE, NULL);

	if(g_file_query_exists(executable_flag_file, NULL)) {
		char buffer[ARG_MAX];
		GFileInputStream * in = g_file_read(executable_flag_file, NULL, NULL);
		gsize read_size = 0;
		gboolean success = g_input_stream_read_all(G_INPUT_STREAM(in), buffer, ARG_MAX, &read_size, NULL, NULL);
		if(success && read_size <= ARG_MAX - 1) {
			return strdup(buffer);
		} else {
			return NULL;
		}
	} else {
		return strdup(get_default_game_executable(appid));
	}
}

const char * get_default_game_executable(int appid) {
	const int game_id = steam_game_id_from_app_id(appid);
	//unknown game
	if(game_id == -1)
		return NULL;
	return GAMES_EXECUTABLE[game_id];
}
