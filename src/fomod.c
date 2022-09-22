#include "fomod.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>


//TODO:cleanup prop
//and other strings

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
	if(strcmp(order, "Explicit") == 0) {
		return ORD;
	} else if(strcmp(order, "Descending") == 0) {
		return DESC;
	} else if(strcmp(order, "Ascending") == 0) {
		return ASC;
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

FOModGroup_t * parseGroup(xmlNodePtr groupNode) {
	xmlNodePtr pluginsNode = groupNode->children;
	FOModGroup_t * group = malloc(sizeof(FOModGroup_t));
	group->name = (char *) xmlGetProp( groupNode, (const xmlChar *) "name");
	group->type = getGroupType((char *) xmlGetProp(groupNode, (const xmlChar *) "type"));
	group->order = getFOModOrder((char *) xmlGetProp(pluginsNode, (const xmlChar *) "order"));
	FOModPlugin_t * plugins = NULL;
	int pluginCount = 0;

	xmlNodePtr currentPlugin = pluginsNode->children;
	while(currentPlugin != NULL) {
		pluginCount += 1;
		plugins = realloc(plugins, pluginCount * sizeof(FOModPlugin_t));
		FOModPlugin_t * plugin = &plugins[pluginCount - 1];

		plugin->name = (char *) xmlGetProp(currentPlugin, (const xmlChar *) "name");

		xmlNodePtr nodeElement = currentPlugin->children;
		while(nodeElement != NULL) {
			if(xmlStrcmp(nodeElement->name, (const xmlChar *) "description") == 0) {
				plugin->description = (char *) xmlNodeGetContent(nodeElement);
			} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "image") == 0) {
				plugin->image = (char *) xmlGetProp(currentPlugin, (const xmlChar *) "path");
			} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "conditionFlags") == 0) {
				FOModFlag_t * flags = NULL;
				int flagCount = 0;
				xmlNodePtr flagNode = nodeElement->children;
				while(flagNode != NULL) {
					flagCount += 1;
					flags = realloc(flags, flagCount * sizeof(FOModFlag_t));

					flags[flagCount - 1].name = (char *) xmlGetProp(nodeElement, (const xmlChar *) "name");
					flags[flagCount - 1].value = (char *) xmlNodeGetContent(nodeElement);

					flagNode = flagNode->next;
				}
				plugin->flags = flags;
			} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "files") == 0) {
				FOModFile_t * files = NULL;
				int filesCount = 0;
				xmlNodePtr flagNode = nodeElement->children;
				while(flagNode != NULL) {
					filesCount += 1;

					files = realloc(files, (filesCount + 1) * sizeof(FOModFile_t));
					files[filesCount - 1].destination = (char *) xmlGetProp(nodeElement, (const xmlChar *) "destination");
					files[filesCount - 1].source = (char *) xmlGetProp(nodeElement, (const xmlChar *) "source");
					files[filesCount - 1].priority = atoi((char *) xmlGetProp(nodeElement, (const xmlChar *) "priority"));
					files[filesCount - 1].isFolder = xmlStrcmp(nodeElement->name, (const xmlChar *) "folder") == 0;

					flagNode = flagNode->next;
				}
				plugin->files = files;
			} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "typeDescriptor") == 0) {
				plugin->type = getDescriptor((char *) xmlGetProp(nodeElement->children, (const xmlChar *) "name"));
			}
		}


		currentPlugin = currentPlugin->next;
	}

	group->plugins = plugins;
	group->pluginCount = pluginCount;
	return group;
}

FOModStep_t * parseInstallSteps(xmlNodePtr installStepsNode, int * stepCount) {
	FOModStep_t * steps = NULL;
	*stepCount = 0;

	xmlNodePtr stepNode = installStepsNode->children;
	while(stepNode != NULL) {
		stepCount += 1;
		steps = realloc(steps, *stepCount * sizeof(FOModStep_t));

		FOModStep_t * step = &steps[*stepCount - 1];
		step->name = (char *)xmlGetProp(stepNode, (const xmlChar *)"name");
		step->requiredFlags = NULL;
		step->flagCount = 0;

		xmlNodePtr stepchildren = stepNode->children;

		while(stepchildren != NULL) {
			if(xmlStrcmp(stepchildren->name, (const xmlChar *) "visible") == 0) {
				xmlNodePtr requiredFlagsNode = stepchildren->children;

				while (requiredFlagsNode != NULL) {
					step->flagCount += 1;
					step->requiredFlags = realloc(step->requiredFlags, step->flagCount * sizeof(FOModFlag_t));
					FOModFlag_t * flag = &(step->requiredFlags[step->flagCount - 1]);
					flag->name = (char *) xmlGetProp(requiredFlagsNode, (const xmlChar *) "flag");
					flag->value = (char *) xmlGetProp(requiredFlagsNode, (const xmlChar *) "value");

					requiredFlagsNode = requiredFlagsNode->next;
				}

			} else if(xmlStrcmp(stepchildren->name, (const xmlChar *) "optionalFileGroups") == 0) {
				if(stepchildren != NULL) {
					step->optionOrder = getFOModOrder((const char *)xmlGetProp(stepchildren, (const xmlChar *)"order"));
					xmlNodePtr group = stepchildren->children;
					step->groups = parseGroup(group);
				}
			}

			stepchildren = stepchildren->next;
		}



		stepNode = stepNode->next;
	}
	return steps;
}

void parseFOMod(const char * fomodFile, FOMod_t* fomod) {
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(fomodFile);

	if(doc == NULL) {
		fprintf(stderr, "Document not parsed successfully");
		return;
	}


	cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		fprintf(stderr, "emptyDocument");
		xmlFreeDoc(doc);
		return;
	}

	if(xmlStrcmp(cur->name, (const xmlChar *) "config") == 0) {
		fprintf(stderr, "document of the wrong type, root node != config");
	}

	cur = cur->children;
	while(cur != NULL) {
		if(xmlStrcmp(cur->name, (const xmlChar *) "moduleName") == 0) {
			//might cause some issues. when will c finally support utf-8
			fomod->moduleName = (char *)cur->content;
		} else if(xmlStrcmp(cur->name, (const xmlChar *) "moduleImage") == 0) {
			xmlChar * imagePath = xmlGetProp(cur, (const xmlChar *)"path");
			fomod->moduleImage = strdup((const char *)imagePath);
			xmlFree(imagePath);
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"requiredInstallFiles") == 0) {
			//TODO: support non empty destination.
			xmlNodePtr requiredInstallFile = cur->children;
			while(requiredInstallFile != NULL) {
				int size = countUntilNull(fomod->requiredInstallFiles) + 2;
				fomod->requiredInstallFiles = realloc(fomod->requiredInstallFiles, sizeof(char *) * size);
				//ensure it is null terminated
				fomod->requiredInstallFiles[size - 1] = NULL;

				//we remove all link with libxml2
				xmlChar * source = xmlGetProp(requiredInstallFile, (const xmlChar *)"source");
				fomod->requiredInstallFiles[size - 2] = strdup((const char *)source);
				xmlFree(source);

				requiredInstallFile = cur->children;
			}
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"installSteps") == 0) {
			xmlChar * stepOrder = xmlGetProp(cur, (xmlChar *)"order");
			fomod->stepOrder = getFOModOrder((char *)stepOrder);
			xmlFree(stepOrder);

			int stepCount = 0;
			FOModStep_t * steps = parseInstallSteps(cur, &stepCount);

			fomod->steps = steps;
			fomod->stepCount = stepCount;
		} else if(xmlStrcmp(cur->name, (const xmlChar *) "conditionalFileInstalls") == 0) {
			fomod->condFiles = NULL;
			fomod->condFilesCount = 0;

			xmlNodePtr patterns = cur->children;
			if(patterns != NULL) {
				xmlNodePtr currentPattern = patterns->children;

				while(currentPattern != NULL) {
					xmlNodePtr patternChild = currentPattern->children;

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

							while(flagNode != NULL) {
								condFile->flagCount += 1;
								condFile->requiredFlags = realloc(condFile->requiredFlags, condFile->flagCount * sizeof(FOModFlag_t));
								FOModFlag_t * flag = &(condFile->requiredFlags[condFile->flagCount - 1]);
								flag->name = (char *) xmlGetProp(flagNode, (const xmlChar *) "flag");
								flag->value = (char *) xmlGetProp(flagNode, (const xmlChar *) "value");

								flagNode = flagNode->next;
							}


						} else if(xmlStrcmp(patternChild->name, (xmlChar *)"files") == 0) {
							xmlNodePtr filesNode = patternChild->children;

							while(filesNode != NULL) {
								condFile->fileCount += 1;
								condFile->files = realloc(condFile->files, condFile->fileCount * sizeof(FOModFile_t));
								FOModFile_t * flag = &(condFile->files[condFile->fileCount - 1]);
								flag->source = (char *) xmlGetProp(filesNode, (const xmlChar *) "source");
								flag->destination = (char *) xmlGetProp(filesNode, (const xmlChar *) "destination");
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
