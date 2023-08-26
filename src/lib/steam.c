#include "steam.h"
#include "macro.h"
#include "getHome.h"

#include <constants.h>

#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <wordexp.h>

enum FieldIds { FIELD_PATH, FIELD_LABEL, FIELD_CONTENT_ID, FIELD_TOTAL_SIZE, FIELD_CLEAN_BYTES, FIELD_CORRUPTION, FIELD_APPS };

// relative to the home directory
static const char * steamLibraries[] = {
	"/.steam/root/",
	"/.steam/steam/",
	"/.local/share/steam",
	//flatpack steam.
	"/.var/app/com.valvesoftware.Steam/.local/share/Steam"
};

static int get_filed_id(const char * field) {
	//replace with a hash + switch
	if(strcmp(field, "path") == 0) {
		return FIELD_PATH;
	} else if(strcmp(field, "label") == 0) {
		return FIELD_LABEL;
	} else if(strcmp(field, "contentid") == 0) {
		return FIELD_CONTENT_ID;
	} else if(strcmp(field, "totalsize") == 0) {
		return FIELD_TOTAL_SIZE;
	} else if(strcmp(field, "update_clean_bytes_tally") == 0) {
		return FIELD_CLEAN_BYTES;
	} else if(strcmp(field, "time_last_update_corruption") == 0) {
		return FIELD_CORRUPTION;
	} else if(strcmp(field, "apps") == 0) {
		return FIELD_APPS;
	} else {
		g_error( "unknown field\n");
		return -1;
	}
}

static SteamLibraries_t * parse_vdf(GFile * file_path, size_t * size, int * status) {
	g_autofree const char * path = g_file_get_path(file_path);
	FILE * fd = fopen(path, "r");
	char * line = NULL;
	size_t len = 0;

	*status = EXIT_SUCCESS;

	bool in_quotes = false;

	SteamLibraries_t * libraries = NULL;
	*size = 0;

	//skip the "libraryfolders" label & the first opening brace
	getline(&line, &len, fd);
	getline(&line, &len, fd);

	int brace_depth = 0;

	//can support up to 1024 car in quotes;
	char buffer[1024];
	bool is_app_id = TRUE;
	int buffer_index = 0;
	int next_field_to_fill = -1;

	while (getline(&line, &len, fd) != -1) {
		for(int i = 0; line[i] != '\0'; i++) {
			switch (line[i]) {
			case '"':
				if(brace_depth == 0) {
					//this is a library id so we don't care.
					buffer_index = 0;
				} else {
					//actual data
					if(in_quotes) {
						in_quotes = FALSE;
						buffer[buffer_index] = '\0';
						if(next_field_to_fill == -1) {
							next_field_to_fill = get_filed_id(buffer);
							if(next_field_to_fill == -1) {
								if(libraries != NULL) free(libraries);
								libraries = NULL;
								goto exit;
							}

						} else {
							char * value = strndup(buffer, buffer_index);
							SteamLibraries_t * library = &libraries[*size - 1];
							switch (next_field_to_fill) {
							case FIELD_PATH:
								library->path = value;
								break;

							case FIELD_LABEL:
								library->label = value;
								break;

							case FIELD_CONTENT_ID:
								library->contentId = value;
								break;

							case FIELD_TOTAL_SIZE:
								library->totalSize = strtoul(value, NULL, 0);
								free(value);
								break;

							case FIELD_CLEAN_BYTES:
								library->update_clean_bytes_tally = value;
								break;

							case FIELD_CORRUPTION:
								library->time_last_update_corruption = value;
								break;

							case FIELD_APPS:
								if(is_app_id) {
									library->apps_count++;
									library->apps = realloc(library->apps, library->apps_count * sizeof(SteamApp_t));
									unsigned int appid = strtoul(value, NULL, 10);
									library->apps[library->apps_count - 1].appid = appid;
								} else {
									library->apps[library->apps_count - 1].update = strtoul(value, NULL, 10);
								}
								free(value);
								is_app_id = !is_app_id;
								break;
							default:
								free(value);
							}
							//field apps is using braces so the syntax is different
							if(next_field_to_fill != FIELD_APPS)next_field_to_fill = -1;
						}
					} else {
						in_quotes = TRUE;
					}
					buffer_index = 0;

				}
				break;
			case '{':
				brace_depth++;
				if(brace_depth == 1) {
					*size += 1;
					libraries = realloc(libraries, sizeof(SteamLibraries_t) * (*size));
					memset(&libraries[*size - 1], 0, sizeof(SteamLibraries_t));
				}
				break;
			case '}':
				if(in_quotes) {
					g_error( "Syntax error in VDF\n");
					//TODO: fix this leak
					free(libraries);
					libraries = NULL;
					*status = EXIT_FAILURE;
					goto exit;
				}
				if(next_field_to_fill == FIELD_APPS) {
					next_field_to_fill = -1;
				}
				brace_depth--;
				break;

			default:
				if(in_quotes) {
					buffer[buffer_index++] = line[i];
				}
			}
		}
	}

exit:
	if(line != NULL) free(line);
	fclose(fd);
	return libraries;
}

static void freeLibraries(SteamLibraries_t * libraries, int size) {
	for(int i = 0; i < size; i++) {
		free(libraries[i].path);
		free(libraries[i].label);
		free(libraries[i].contentId);
		free(libraries[i].update_clean_bytes_tally);
		free(libraries[i].time_last_update_corruption);
		free(libraries[i].apps);
	}
	free(libraries);
}

static GHashTable* game_table_singleton = NULL;

void steam_freeGameTable() {
	if(game_table_singleton != NULL)g_hash_table_destroy(game_table_singleton);
	game_table_singleton = NULL;
}

error_t steam_search_games(GHashTable ** p_hash_table) {
	if(game_table_singleton != NULL) {
		*p_hash_table = game_table_singleton;
		return ERR_SUCCESS;
	}

	SteamLibraries_t * libraries = NULL;
	size_t size = 0;
	GFile * home = audit_get_home();
	g_autofree char * home_path = g_file_get_path(home);

	for(unsigned long i = 0; i < LEN(steamLibraries); i++) {
		GFile * path = g_file_new_build_filename(home_path, steamLibraries[i], "steamapps/libraryfolders.vdf", NULL);
		if (g_file_query_exists(path, NULL)) {
			int parserStatus;
			libraries = parse_vdf(path, &size, &parserStatus);
			if(parserStatus == EXIT_SUCCESS) {
				g_free(path);
				break;
			}
		}

		g_free(path);
	}

	free(home);
	if(libraries == NULL) {
		return ERR_FAILURE;
	}

	GHashTable* table = g_hash_table_new_full(g_int_hash, g_int_equal, free, free);

	//fill the table
	for(unsigned long i = 0; i < size; i++) {
		for(unsigned long j = 0; j < libraries[i].apps_count; j++) {
			int game_id = steam_game_id_from_app_id(libraries[i].apps[j].appid);
			if(game_id >= 0) {
				int * key = g_malloc(sizeof(int));
				*key = game_id;
				g_hash_table_insert(table, key, strdup(libraries[i].path));
			}
		}
	}

	freeLibraries(libraries, size);
	*p_hash_table = table;
	game_table_singleton = table;
	return ERR_SUCCESS;
}


int steam_game_id_from_app_id(u_int32_t appid) {
	for(unsigned long k = 0; k < LEN(GAMES_APPIDS); k++) {
		if(appid == GAMES_APPIDS[k]) {
			return k;
		}
	}
	return -1;
}

int steam_parseAppId(const char * appid_str) {
	GHashTable * gamePaths;
	error_t status = steam_search_games(&gamePaths);
	if(status == ERR_FAILURE) {
		return -1;
	}

	char * strtoulSentinel;
	//strtoul set EINVAL(after C99) if the string is invalid
	unsigned long appid = strtoul(appid_str, &strtoulSentinel, 10);
	if(errno == EINVAL || strtoulSentinel == appid_str) {
		g_error( "Appid has to be a valid number\n");
		return -1;
	}

	int gameId = steam_game_id_from_app_id((int)appid);
	if(gameId < 0) {
		g_error( "Game is not compatible\n");
		return -1;
	}

	if(!g_hash_table_contains(gamePaths, &gameId)) {
		g_error( "Game not found\n");
		return -1;
	}

	//no valid appid goes far enough to justify long
	return (int)appid;
}
