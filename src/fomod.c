#include "fomod.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <glib.h>
#include <stdarg.h>
#include "file.h"
#include "libxml/globals.h"

//TODO:cleanup prop
//and other strings
//TODO: protect every access to a node's child to avoid reading the wrong node


//replace \ in the path by /
void fixPath(char * path) {
	while(*path != '\0') {
		if(*path == '\\')*path = '/';
		path++;
	}
}

int countUntilNull(void * pointers) {
	int i = 0;
	while(pointers != NULL) {
		pointers++;
		i++;
	}
	return i;
}

GroupType_t getGroupType(const char * type) {
	if(strcmp(type, "SelectAtLeastOne") == 0) {
		return AT_LEAST_ONE;
	} else if(strcmp(type, "SelectAtMostOne") == 0) {
		return AT_MOST_ONE;
	} else if(strcmp(type, "SelectExactlyOne") == 0) {
		return ONE_ONLY;
	} else if(strcmp(type, "SelectAll") == 0) {
		return ALL;
	} else if(strcmp(type, "SelectAny") == 0) {
		return ANY;
	} else {
		return -1;
	}
}

FOModOrder_t getFOModOrder(const char * order) {
	if(order == NULL || strcmp(order, "Ascending") == 0) {
		return ASC;
	} else if(strcmp(order, "Explicit") == 0) {
		return ORD;
	} else if(strcmp(order, "Descending") == 0) {
		return DESC;
	}
	return -1;
}

TypeDescriptor_t getDescriptor(const char * descriptor) {
	if(strcmp(descriptor, "Optional") == 0) {
		return OPTIONAL;
	} else if(strcmp(descriptor, "Required") == 0) {
		return REQUIRED;
	} else if(strcmp(descriptor, "Recommended") == 0) {
		return RECOMMENDED;
	} else if(strcmp(descriptor, "NotUsable") == 0) {
		return MAYBE_USABLE;
	} else if(strcmp(descriptor, "NotUsable") == 0) {
		return NOT_USABLE;
	} else {
		return -1;
	}
}


//names cannot contain false
//need to be null terminated
bool validateNode(xmlNodePtr * node, bool skipText, const char * names, ...) {
	va_list namesPtr;

	while(*node != NULL && xmlStrcmp((*node)->name, (const xmlChar *)"text") == 0) {
		if(skipText) {
			(*node) = (*node)->next;
		} else {
			//could not skip and the node was a text node.
			return false;
		}
	}

	//TODO: chose a proper return value
	if(*node == NULL) {
		return true;
	}

	va_start(namesPtr, names);

	const char * validName = names;
	while(validName != NULL) {
		if(xmlStrcmp((*node)->name, (const xmlChar *)validName) == 0) {
			va_end(namesPtr);
			return true;
		}
		validName = va_arg(namesPtr, char *);
	}

	va_end(namesPtr);
	return false;
}




char * freeAndDup(xmlChar * line) {
	char * free = strdup((const char *) line);
	xmlFree(line);
	return free;
}

FOModGroup_t parseGroup(xmlNodePtr groupNode) {
	xmlNodePtr pluginsNode = groupNode->children;

	if(!validateNode(&pluginsNode, true, "plugins", NULL)) {
		//TODO handle error;
		printf("%d\n", __LINE__);
		exit(EXIT_FAILURE);
	}


	FOModGroup_t group;
	group.name = freeAndDup(xmlGetProp( groupNode, (const xmlChar *) "name"));

	xmlChar * type = xmlGetProp(groupNode, (const xmlChar *) "type");
	group.type = getGroupType((const char *)type);
	xmlFree(type);

	char * order = (char *) xmlGetProp(pluginsNode, (const xmlChar *) "order");
	group.order = getFOModOrder(order);
	xmlFree(order);

	FOModPlugin_t * plugins = NULL;
	int pluginCount = 0;

	xmlNodePtr currentPlugin = pluginsNode->children;
	while(currentPlugin != NULL) {
		if(!validateNode(&currentPlugin, true, "plugin", NULL)) {
			//TODO handle error;
			printf("%d\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		if(currentPlugin == NULL)continue;


		pluginCount += 1;
		plugins = realloc(plugins, pluginCount * sizeof(FOModPlugin_t));
		FOModPlugin_t * plugin = &plugins[pluginCount - 1];
		//initialise everything to 0 and null pointers
		memset(plugin, 0, sizeof(FOModPlugin_t));

		plugin->name = freeAndDup(xmlGetProp(currentPlugin, (const xmlChar *) "name"));

		xmlNodePtr nodeElement = currentPlugin->children;
		while(nodeElement != NULL) {
			if(xmlStrcmp(nodeElement->name, (const xmlChar *) "description") == 0) {
				plugin->description = freeAndDup(xmlNodeGetContent(nodeElement));
			} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "image") == 0) {
				plugin->image = freeAndDup(xmlGetProp(currentPlugin, (const xmlChar *) "path"));
			} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "conditionFlags") == 0) {
				FOModFlag_t * flags = NULL;
				int flagCount = 0;
				xmlNodePtr flagNode = nodeElement->children;
				while(flagNode != NULL) {
					if(!validateNode(&flagNode, true, "flag", NULL)) {
						//TODO: handle error
						printf("%d\n", __LINE__);
						exit(EXIT_FAILURE);
					}

					if(flagNode == NULL)continue;

					flagCount += 1;
					flags = realloc(flags, flagCount * sizeof(FOModFlag_t));

					flags[flagCount - 1].name = freeAndDup(xmlGetProp(flagNode, (const xmlChar *) "name"));
					flags[flagCount - 1].value = freeAndDup(xmlNodeGetContent(flagNode));

					flagNode = flagNode->next;
				}
				plugin->flags = flags;
				plugin->flagCount = flagCount;
			} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "files") == 0) {
				FOModFile_t * files = NULL;
				int filesCount = 0;
				xmlNodePtr fileNode = nodeElement->children;
				while(fileNode != NULL) {
					if(!validateNode(&fileNode, true, "folder", "file", NULL)) {
						//TODO: handle error
						printf("%d\n", __LINE__);
						exit(EXIT_FAILURE);
					}

					if(fileNode == NULL)continue;


					filesCount += 1;

					files = realloc(files, (filesCount + 1) * sizeof(FOModFile_t));
					files[filesCount - 1].destination = freeAndDup(xmlGetProp(fileNode, (const xmlChar *) "destination"));
					files[filesCount - 1].source = freeAndDup(xmlGetProp(fileNode, (const xmlChar *) "source"));
					//TODO: test if it's a number
					//TODO: test if props is present and default to 0
					xmlChar * priority = xmlGetProp(fileNode, (const xmlChar *) "priority");
					if(priority == NULL) {
						files[filesCount - 1].priority = 0;
					} else {
						files[filesCount - 1].priority = atoi((char *)priority);
					}
					xmlFree(priority);
					files[filesCount - 1].isFolder = xmlStrcmp(fileNode->name, (const xmlChar *) "folder") == 0;

					fileNode = fileNode->next;
				}
				plugin->files = files;
				plugin->fileCount = filesCount;
			} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "typeDescriptor") == 0) {
				xmlNodePtr typeNode = nodeElement->children;
				if(!validateNode(&typeNode, true, "type", NULL)) {
					//TODO: handle error
					printf("%d\n", __LINE__);
					exit(EXIT_FAILURE);
				}
				xmlChar * name = xmlGetProp(typeNode, (const xmlChar *) "name");
				plugin->type = getDescriptor((char *) name);
				xmlFree(name);
			}

			nodeElement = nodeElement->next;
		}


		currentPlugin = currentPlugin->next;
	}

	group.plugins = plugins;
	group.pluginCount = pluginCount;
	return group;
}

FOModStep_t * parseInstallSteps(xmlNodePtr installStepsNode, int * stepCount) {
	FOModStep_t * steps = NULL;
	*stepCount = 0;

	xmlNodePtr stepNode = installStepsNode->children;
	while(stepNode != NULL) {
		//skipping the text node
		if(!validateNode(&stepNode, true, "installStep", NULL)) {
			//TODO: handle error
			printf("%d\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		if(stepNode == NULL)break;

		*stepCount += 1;
		steps = realloc(steps, *stepCount * sizeof(FOModStep_t));

		FOModStep_t * step = &steps[*stepCount - 1];
		step->name = freeAndDup(xmlGetProp(stepNode, (const xmlChar *)"name"));
		step->requiredFlags = NULL;
		step->flagCount = 0;
		step->groupCount = 0;
		step->groups = NULL;

		xmlNodePtr stepchildren = stepNode->children;

		while(stepchildren != NULL) {
			if(xmlStrcmp(stepchildren->name, (const xmlChar *) "visible") == 0) {
				xmlNodePtr requiredFlagsNode = stepchildren->children;

				while (requiredFlagsNode != NULL) {

					if(!validateNode(&requiredFlagsNode, true, "flagDependency", NULL)) {
						//TODO: handle error
						printf("%d\n", __LINE__);
						exit(EXIT_FAILURE);
					}

					if(requiredFlagsNode == NULL)break;

					step->flagCount += 1;
					step->requiredFlags = realloc(step->requiredFlags, step->flagCount * sizeof(FOModFlag_t));
					FOModFlag_t * flag = &(step->requiredFlags[step->flagCount - 1]);
					flag->name = freeAndDup(xmlGetProp(requiredFlagsNode, (const xmlChar *) "flag"));
					flag->value = freeAndDup(xmlGetProp(requiredFlagsNode, (const xmlChar *) "value"));

					requiredFlagsNode = requiredFlagsNode->next;
				}

			} else if(xmlStrcmp(stepchildren->name, (const xmlChar *) "optionalFileGroups") == 0) {

				xmlChar * optionOrder = xmlGetProp(stepchildren, (const xmlChar *)"order");
				step->optionOrder = getFOModOrder((char *)optionOrder);
				xmlFree(optionOrder);
				//skipping the text node;
				xmlNodePtr group = stepchildren->children;
				while(group != NULL) {
					if(!validateNode(&group, true, "group", NULL)) {
						//TODO: handle error
						printf("%d\n", __LINE__);
						exit(EXIT_FAILURE);
					}

					if(group == NULL)break;

					step->groupCount += 1;
					step->groups = realloc(step->groups, step->groupCount * sizeof(FOModGroup_t));
					step->groups[step->groupCount - 1] = parseGroup(group);

					group = group->next;
				}
			}

			stepchildren = stepchildren->next;
		}



		stepNode = stepNode->next;
	}
	return steps;
}

int parseFOMod(const char * fomodFile, FOMod_t* fomod) {
	xmlDocPtr doc;
	xmlNodePtr cur;

	fomod->condFiles = NULL;
	fomod->condFilesCount = 0;
	fomod->requiredInstallFiles = NULL;
	fomod->stepCount = 0;
	fomod->stepOrder = 0;
	fomod->steps = NULL;
	fomod->moduleImage = NULL;
	fomod->moduleName = NULL;

	doc = xmlParseFile(fomodFile);

	if(doc == NULL) {
		fprintf(stderr, "Document is not a valid xml file\n");
		return EXIT_FAILURE;
	}


	cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		fprintf(stderr, "emptyDocument");
		xmlFreeDoc(doc);
		return EXIT_FAILURE;
	}

	if(xmlStrcmp(cur->name, (const xmlChar *) "config") != 0) {
		fprintf(stderr, "document of the wrong type, root node is '%s' instead of config\n", cur->name);
		return EXIT_FAILURE;
	}

	cur = cur->children;
	while(cur != NULL) {
		if(xmlStrcmp(cur->name, (const xmlChar *) "moduleName") == 0) {
			//might cause some issues. when will c finally support utf-8
			fomod->moduleName = (char *)cur->content;
		} else if(xmlStrcmp(cur->name, (const xmlChar *) "moduleImage") == 0) {
			fomod->moduleImage = freeAndDup(xmlGetProp(cur, (const xmlChar *)"path"));
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"requiredInstallFiles") == 0) {
			//TODO: support non empty destination.
			xmlNodePtr requiredInstallFile = cur->children;
			while(requiredInstallFile != NULL) {
				if(validateNode(&requiredInstallFile, true, "folder", "file", NULL)) {
					//TODO: handle error
					printf("%d\n", __LINE__);
					exit(EXIT_FAILURE);
				}

				int size = countUntilNull(fomod->requiredInstallFiles) + 2;
				fomod->requiredInstallFiles = realloc(fomod->requiredInstallFiles, sizeof(char *) * size);
				//ensure it is null terminated
				fomod->requiredInstallFiles[size - 1] = NULL;
				fomod->requiredInstallFiles[size - 2] = freeAndDup(xmlGetProp(requiredInstallFile, (const xmlChar *)"source"));

				requiredInstallFile = cur->children;
			}
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"installSteps") == 0) {
			xmlChar * stepOrder = xmlGetProp(cur, (xmlChar *)"order");
			fomod->stepOrder = getFOModOrder((char *)stepOrder);
			xmlFree(stepOrder);

			int stepCount = 0;
			FOModStep_t * steps = parseInstallSteps(cur, &stepCount);

			if(steps == NULL) {
				fprintf(stderr, "Failed to parse the install steps\n");

				//TODO: manage the error properly
				return EXIT_FAILURE;
			}

			fomod->steps = steps;
			fomod->stepCount = stepCount;
		} else if(xmlStrcmp(cur->name, (const xmlChar *) "conditionalFileInstalls") == 0) {
			xmlNodePtr patterns = cur->children;
			if(patterns != NULL) {
				if(!validateNode(&patterns, true, "patterns", NULL)) {
					//TODO: handle error
					printf("%d\n", __LINE__);
					return EXIT_FAILURE;
				}
				xmlNodePtr currentPattern = patterns->children;
				while(currentPattern != NULL) {
					xmlNodePtr patternChild = currentPattern->children;

					if(!validateNode(&patternChild, true, "pattern", NULL)) {
						//TODO: handle error
						printf("%d\n", __LINE__);
						return EXIT_FAILURE;
					}

					while(patternChild != NULL) {
						fomod->condFilesCount += 1;
						fomod->condFiles = realloc(fomod->condFiles, fomod->condFilesCount * sizeof(FOModCondFile_t));
						FOModCondFile_t * condFile = &(fomod->condFiles[fomod->condFilesCount - 1]);

						condFile->fileCount = 0;
						condFile->files = NULL;
						condFile->flagCount = 0;
						condFile->requiredFlags = NULL;

						if(xmlStrcmp(patternChild->name, (xmlChar *)"dependencies") == 0) {
							xmlNodePtr flagNode = patternChild->children;

							if(!validateNode(&flagNode, true, "flagDependency", NULL)) {
								//TODO: handle error
								printf("%d\n", __LINE__);
								return EXIT_FAILURE;
							}

							while(flagNode != NULL) {
								condFile->flagCount += 1;
								condFile->requiredFlags = realloc(condFile->requiredFlags, condFile->flagCount * sizeof(FOModFlag_t));
								FOModFlag_t * flag = &(condFile->requiredFlags[condFile->flagCount - 1]);
								flag->name = freeAndDup(xmlGetProp(flagNode, (const xmlChar *) "flag"));
								flag->value = freeAndDup(xmlGetProp(flagNode, (const xmlChar *) "value"));

								flagNode = flagNode->next;
							}


						} else if(xmlStrcmp(patternChild->name, (xmlChar *)"files") == 0) {
							xmlNodePtr filesNode = patternChild->children;


							while(filesNode != NULL) {
								if(!validateNode(&filesNode, true, "folder", "file", NULL)) {
									//TODO: handle error
									printf("%d\n", __LINE__);
									return EXIT_FAILURE;
								}

								condFile->fileCount += 1;
								condFile->files = realloc(condFile->files, condFile->fileCount * sizeof(FOModFile_t));
								FOModFile_t * flag = &(condFile->files[condFile->fileCount - 1]);
								flag->source = freeAndDup(xmlGetProp(filesNode, (const xmlChar *) "source"));
								flag->destination = freeAndDup(xmlGetProp(filesNode, (const xmlChar *) "destination"));
								flag->priority = 0;
								flag->isFolder = xmlStrcmp(patternChild->name, (xmlChar *) "folder") == 0;

								filesNode = filesNode->next;
							}
						}
						patternChild = patternChild->next;
					}
					currentPattern = currentPattern->next;
				}
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
}

int getInputCount(const char * input) {
	char buff[2];
	buff[1] = '\0';

	int elementCount = 0;
	bool prevWasValue = false;

	for(int i = 0; input[i] != '\0' && input[i] != '\n'; i++) {
		buff[0] = input[i];

		//if the character is a valid character
		if(strstr("0123456789 ", buff) != NULL) {
			if(input[i] == ' ') prevWasValue = false;
			else if(!prevWasValue) {
				prevWasValue = true;
				elementCount++;
			}
		} else {
			return -1;
		}

	}

	return elementCount;
}

char * findFOModFolder(const char * modFolder) {
	struct dirent * dir;
	DIR *d = opendir(modFolder);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(g_ascii_strcasecmp(dir->d_name, "fomod") == 0) {
				char * fomodDir = g_build_path("/", modFolder, dir->d_name, NULL);
				closedir(d);
				return fomodDir;
			}
		}
		closedir(d);
	}
	return NULL;
}

char * findFOModFile(const char * fomodFolder) {
	struct dirent * dir;
	DIR *d = opendir(fomodFolder);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(g_ascii_strcasecmp(dir->d_name, "moduleconfig.xml") == 0) {
				char * fomodFile = g_build_filename(fomodFolder, dir->d_name, NULL);
				closedir(d);
				return fomodFile;
			}
		}
		closedir(d);
	}
	return NULL;
}

void printfOptionsInOrder(FOModGroup_t group) {
	for(int i = 0; i < group.pluginCount; i++) {
		printf("%d, %s\n", i, group.plugins[i].name);
		printf("%s\n", group.plugins[i].description);
	}
}

gint flagEqual(const FOModFlag_t * a, const FOModFlag_t * b) {
	int nameCmp = strcmp(a->name, b->name);
	if(nameCmp == 0) {
		if(strcmp(a->value, b->value) == 0)
			//is equal
			return 0;

		return 1;
	}
	return nameCmp;
}

//Maybe integrate this into the rest of the code instead of freeing after the fact
void freeFOMod(FOMod_t * fomod) {
	for(int i = 0; i < fomod->condFilesCount; i++) {
		FOModCondFile_t * condFile = &(fomod->condFiles[i]);
		for(int fileId = 0; fileId < condFile->fileCount; fileId++) {
			free(condFile->files[fileId].destination);
			free(condFile->files[fileId].source);
		}

		for(int flagId = 0; flagId < condFile->flagCount; flagId++) {
			FOModFlag_t * flag = &(condFile->requiredFlags[flagId]);
			free(flag->name);
			free(flag->value);
		}
		free(condFile->files);
		free(condFile->requiredFlags);
	}
	free(fomod->condFiles);

	free(fomod->moduleImage);
	free(fomod->moduleName);

	int size = countUntilNull(fomod->requiredInstallFiles);
	for(int i = 0; i < size; i++) {
		free(fomod->requiredInstallFiles[i]);
	}
	free(fomod->requiredInstallFiles);

	for(int i = 0; i < fomod->stepCount; i++) {
		FOModStep_t * step = &fomod->steps[i];
		for(int groupId = 0; groupId < step->groupCount; groupId++) {
			FOModGroup_t * group = &step->groups[groupId];
			free(group->name);
			for(int pluginId = 0; pluginId < group->pluginCount; pluginId++) {
				FOModPlugin_t * plugin = &(group->plugins[pluginId]);
				free(plugin->description);
				free(plugin->image);
				free(plugin->name);
				for(int fileId = 0; fileId < plugin->fileCount; fileId++) {
					free(plugin->files[fileId].destination);
					free(plugin->files[fileId].source);
				}

				for(int flagId = 0; flagId < plugin->flagCount; flagId++) {
					free(plugin->flags[flagId].name);
					free(plugin->flags[flagId].value);
				}

				free(plugin->files);
				free(plugin->flags);
			}
			free(step->groups[groupId].plugins);
		}
		for(int flagId = 0; flagId < step->flagCount; flagId++) {
			FOModFlag_t * flag = &(step->requiredFlags[flagId]);
			free(flag->name);
			free(flag->value);
		}
		free(step->requiredFlags);
		free(step->name);
	}
}

//TODO: support order parameters
//TODO: support priority
//TODO: error support
int installFOMod(const char * modFolder, const char * destination) {
	char * fomodFolder = findFOModFolder(modFolder);
	if(fomodFolder == NULL) {
		fprintf(stderr, "Fomod folder not found, are you sure this is a fomod mod ?\n");
		return -1;
	}

	char * fomodFile = findFOModFile(fomodFolder);
	if(fomodFile == NULL) {
		fprintf(stderr, "Fomod file not found, are you sure this is a fomod mod ?\n");
		g_free(fomodFolder);
		return -1;
	}

	FOMod_t fomod;
	int returnValue = parseFOMod(fomodFile, &fomod);
	if(returnValue != EXIT_SUCCESS) {
		return returnValue;
	}

	GList * flagList = NULL;

	for(int i = 0; i < fomod.stepCount; i++) {
		FOModStep_t * step = &fomod.steps[i];

		bool validFlags = true;
		for(int i = 0; i < step->flagCount; i++) {
			GList * flagLink = g_list_find_custom(flagList, &step->requiredFlags[i], (GCompareFunc)flagEqual);
			if(flagLink == NULL) {
				validFlags = false;
				break;
			}
		}

		if(!validFlags) continue;

		for(int groupId = 0; groupId < step->groupCount; groupId++ ) {
			FOModGroup_t group = step->groups[groupId];

			u_int8_t min;
			u_int8_t max;
			int inputCount = 0;
			char *inputBuffer = NULL;
			size_t bufferSize = 0;


			while(true) {
				printfOptionsInOrder(group);
				switch(group.type) {
				case ONE_ONLY:
					printf("Select one :\n");
					min = 1;
					max = 1;
					break;

				case ANY:
					printf("Select any (space separated) (leave empty for nothing) :\n");
					min = 0;
					max = group.pluginCount - 1;
					break;

				case AT_LEAST_ONE:
					printf("Select at least one (space separated) (leave empty for nothing) :\n");
					min = 1;
					max = group.pluginCount - 1;
					break;

				case AT_MOST_ONE:
					printf("Select one or none (space separated) (leave empty for nothing) :\n");
					min = 0;
					max = 1;
					break;

				case ALL:
					printf("Press enter to continue\n");
					min = 0;
					max = 0;
					break;
				}



				if(getline(&inputBuffer, &bufferSize, stdin) > 0) {
					inputCount = getInputCount(inputBuffer);
					if(inputCount <= max && inputCount >= min) {
						printf("Continuing\n");
						break;
					}
					fprintf(stderr, "Invalid input\n");
					if(inputBuffer != NULL)free(inputBuffer);
					inputBuffer = NULL;
				} else {
				//TODO: handle error
				}
			}

			GRegex* regex = g_regex_new("\\s*", 0, 0, NULL);
			char ** choices = g_regex_split(regex, inputBuffer, 0);
			g_regex_unref(regex);

			for(int choiceId = 0; choices[choiceId] != NULL; choiceId++) {
				int choice = atoi(choices[choiceId]);
				FOModPlugin_t plugin = group.plugins[choice];
				for(int i = 0; i < plugin.flagCount; i++) {
					flagList = g_list_append(flagList, &plugin.flags[i]);
				}

				//do the install
				for(int i = 0; i < plugin.fileCount; i++) {
					FOModFile_t * file = &plugin.files[i];

					//TODO: support file->destination
					//TODO: check for copy success
					//no using link since priority is made to override files and link is annoying to deal with when overriding files.
					char * source = (char *) g_build_path("/", modFolder, file->source, NULL);
					fixPath(source);
					int returnValue = EXIT_SUCCESS;
					if(file->isFolder) {
						returnValue = copy(source, destination, CP_NO_TARGET_DIR | CP_RECURSIVE);
					} else {
						returnValue = copy(source, destination, 0);
					}
					printf("source: %s, destination: %s\n", source, destination);
					g_free(source);
					if(returnValue != EXIT_SUCCESS) {
						fprintf(stderr, "Copy failed, some file might be corrupted\n");
						exit(EXIT_FAILURE);
					}
				}
			}

			g_strfreev(choices);
			if(inputBuffer != NULL)free(inputBuffer);
		}
	}

	for(int condId = 0; condId < fomod.condFilesCount; condId++) {
		FOModCondFile_t *condfile = &fomod.condFiles[condId];

		bool areAllFlagsValid = true;

		//check if all flags are valid;
		for(int flagId = 0; flagId < condfile->flagCount; flagId++) {
			GList * link = g_list_find_custom(flagList, &(condfile->requiredFlags[flagId]), (GCompareFunc)flagEqual);
			if(link == NULL) {
				areAllFlagsValid = false;
				break;
			}
		}

		if(areAllFlagsValid) {
			for(int fileId = 0; fileId < condfile->flagCount; fileId++) {
				FOModFile_t * file = &(condfile->files[fileId]);
				//TODO: support destination
				//TODO: handle error
				char * source = (char *) g_build_path("/", modFolder, file->source, NULL);
				fixPath(source);
				int returnValue;
				if(file->isFolder) {
					returnValue = copy(source, destination, CP_NO_TARGET_DIR | CP_RECURSIVE | CP_LINK);
				} else {
					returnValue = copy(source, destination, CP_LINK);
				}
				if(returnValue != EXIT_SUCCESS) {
					fprintf(stderr, "Copy failed, some file might be corrupted\n");
				}
				printf("source: %s, destination: %s\n", source, destination);
				g_free(source);
			}
		}
	}
	printf("FOMod successfully installed!\n");
	g_list_free(flagList);
	freeFOMod(&fomod);
}
