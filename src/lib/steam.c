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

static int getFiledId(const char * field) {
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
		fprintf(stderr, "unknown field\n");
		return -1;
	}
}

static steam_Libraries_t * parseVDF(const char * path, size_t * size, int * status) {
	FILE * fd = fopen(path, "r");
	char * line = NULL;
	size_t len = 0;

	*status = EXIT_SUCCESS;

	bool inQuotes = false;

	steam_Libraries_t * libraries = NULL;
	*size = 0;

	//skip the "libraryfolders" label & the first opening brace
	getline(&line, &len, fd);
	getline(&line, &len, fd);

	int braceDepth = 0;

	//can support up to 1024 car in quotes;
	char buffer[1024];
	bool isAppId = TRUE;
	int bufferIndex = 0;
	int nextFieldToFill = -1;

	while (getline(&line, &len, fd) != -1) {
		for(int i = 0; line[i] != '\0'; i++) {
			switch (line[i]) {
			case '"':
				if(braceDepth == 0) {
					//this is a library id so we don't care.
					bufferIndex = 0;
				} else {
					//actual data
					if(inQuotes) {
						inQuotes = FALSE;
						buffer[bufferIndex] = '\0';
						if(nextFieldToFill == -1) {
							nextFieldToFill = getFiledId(buffer);
							if(nextFieldToFill == -1) {
								if(libraries != NULL) free(libraries);
								libraries = NULL;
								goto exit;
							}

						} else {
							char * value = strndup(buffer, bufferIndex);
							steam_Libraries_t * library = &libraries[*size - 1];
							switch (nextFieldToFill) {
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
								if(isAppId) {
									library->appsCount++;
									library->apps = realloc(library->apps, library->appsCount * sizeof(steam_App_t));
									unsigned int appid = strtoul(value, NULL, 10);
									library->apps[library->appsCount - 1].appid = appid;
								} else {
									library->apps[library->appsCount - 1].update = strtoul(value, NULL, 10);
								}
								free(value);
								isAppId = !isAppId;
								break;
							default:
								free(value);
							}
							//field apps is using braces so the syntax is different
							if(nextFieldToFill != FIELD_APPS)nextFieldToFill = -1;
						}
					} else {
						inQuotes = TRUE;
					}
					bufferIndex = 0;

				}
				break;
			case '{':
				braceDepth++;
				if(braceDepth == 1) {
					*size += 1;
					libraries = realloc(libraries, sizeof(steam_Libraries_t) * (*size));
					memset(&libraries[*size - 1], 0, sizeof(steam_Libraries_t));
				}
				break;
			case '}':
				if(inQuotes) {
					fprintf(stderr, "Syntax error in VDF\n");
					//TODO: fix this leak
					free(libraries);
					libraries = NULL;
					*status = EXIT_FAILURE;
					goto exit;
				}
				if(nextFieldToFill == FIELD_APPS) {
					nextFieldToFill = -1;
				}
				braceDepth--;
				break;

			default:
				if(inQuotes) {
					buffer[bufferIndex++] = line[i];
				}
			}
		}
	}

exit:
	if(line != NULL) free(line);
	fclose(fd);
	return libraries;
}

static void freeLibraries(steam_Libraries_t * libraries, int size) {
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

static GHashTable* gameTableSingleton = NULL;

void steam_freeGameTable() {
	if(gameTableSingleton != NULL)g_hash_table_destroy(gameTableSingleton);
}

error_t steam_searchGames(GHashTable ** p_hashTable) {
	if(gameTableSingleton != NULL) {
		*p_hashTable = gameTableSingleton;
		return ERR_SUCCESS;
	}

	steam_Libraries_t * libraries = NULL;
	size_t size = 0;
	char * home = getHome();

	for(unsigned long i = 0; i < LEN(steamLibraries); i++) {
		char * path = g_build_filename(home, steamLibraries[i], "steamapps/libraryfolders.vdf", NULL);
		if (access(path, F_OK) == 0) {
			int parserStatus;
			libraries = parseVDF(path, &size, &parserStatus);
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
		for(unsigned long j = 0; j < libraries[i].appsCount; j++) {
			int gameId = steam_gameIdFromAppId(libraries[i].apps[j].appid);
			if(gameId >= 0) {
				int * key = malloc(sizeof(int));
				*key = gameId;
				g_hash_table_insert(table, key, strdup(libraries[i].path));
			}
		}
	}

	freeLibraries(libraries, size);
	*p_hashTable = table;
	gameTableSingleton = table;
	return ERR_SUCCESS;
}


int steam_gameIdFromAppId(u_int32_t appid) {
	for(unsigned long k = 0; k < LEN(GAMES_APPIDS); k++) {
		if(appid == GAMES_APPIDS[k]) {
			return k;
		}
	}
	return -1;
}

int steam_parseAppId(const char * appIdStr) {
	GHashTable * gamePaths;
	error_t status = steam_searchGames(&gamePaths);
	if(status == ERR_FAILURE) {
		return -1;
	}

	char * strtoulSentinel;
	//strtoul set EINVAL(after C99) if the string is invalid
	unsigned long appid = strtoul(appIdStr, &strtoulSentinel, 10);
	if(errno == EINVAL || strtoulSentinel == appIdStr) {
		fprintf(stderr, "Appid has to be a valid number\n");
		return -1;
	}

	int gameId = steam_gameIdFromAppId((int)appid);
	if(gameId < 0) {
		fprintf(stderr, "Game is not compatible\n");
		return -1;
	}

	if(!g_hash_table_contains(gamePaths, &gameId)) {
		fprintf(stderr, "Game not found\n");
		return -1;
	}

	//no valid appid goes far enough to justify long
	return (int)appid;
}
