#include "steam.h"
#include "macro.h"
#include "getHome.h"
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <wordexp.h>

//TODO: optimise and replace reallocs

enum FieldIds { FIELD_PATH, FIELD_LABEL, FIELD_CONTENT_ID, FIELD_TOTAL_SIZE, FIELD_CLEAN_BYTES, FIELD_CORRUPTION, FIELD_APPS };

int getFiledId(const char * field) {
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
		printf("unknown field");
		exit(EXIT_FAILURE);
	}
}

ValveLibraries_t * parseVDF(const char * path, size_t * size) {
	FILE * fd = fopen(path, "r");
	char * line = NULL;
	size_t len = 0;
	ssize_t read;



	bool inQuotes = false;

	ValveLibraries_t * libraries = NULL;
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

						} else {
							char * value = strndup(buffer, bufferIndex);
							ValveLibraries_t * library = &libraries[*size - 1];
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
									library->apps = realloc(library->apps, library->appsCount * sizeof(ValveApp_t));
									unsigned int appid = strtoul(value, NULL, 10);
									library->apps[library->appsCount - 1].appid = appid;
								} else {
									library->apps[library->appsCount - 1].update = strtoul(value, NULL, 10);
								}
								isAppId = !isAppId;
								break;
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
					libraries = realloc(libraries, sizeof(ValveLibraries_t) * (*size));
					memset(&libraries[*size - 1], 0, sizeof(ValveLibraries_t));
				}
				break;
			case '}':
				if(inQuotes) {
					printf("Syntax error in VDF\n");
					exit(EXIT_FAILURE);
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

	fclose(fd);
	return libraries;
}

GHashTable* search_games() {
	ValveLibraries_t * libraries = NULL;
	size_t size = 0;
	const char * HOME = getHome();

	for(int i = 0; i < LEN(steamLibraries); i++) {
		char * path = g_build_filename(HOME, steamLibraries[i], "steamapps/libraryfolders.vdf", NULL);
		if (access(path, F_OK) == 0) {
			libraries = parseVDF(path, &size);
			break;
		}

		g_free(path);
	}

	GHashTable* table = g_hash_table_new(g_int_hash, g_int_equal);

	//fill the table
	for(int i = 0; i < size; i++) {
		for(int j = 0; j < libraries[i].appsCount; j++) {
			int gameId = getGameIdFromAppId(libraries[i].apps[j].appid);
			if(gameId >= 0) {
				int * key = malloc(sizeof(int));
				*key = gameId;
				g_hash_table_insert(table, key, strdup(libraries[i].path));
			}
		}
	}

	//free used up memory
	for(int i = 0; i < size; i++) {
		free(libraries[i].path);
		free(libraries[i].label);
		free(libraries[i].contentId);
		free(libraries[i].update_clean_bytes_tally);
		free(libraries[i].time_last_update_corruption);
		free(libraries[i].apps);
	}

	if(libraries != NULL) free(libraries);
	return table;
}


int getGameIdFromAppId(u_int32_t id) {
	for(int k = 0; k < LEN(GAMES_APPIDS); k++) {
		if(id == GAMES_APPIDS[k]) {
			return k;
		}
	}
	return -1;
}