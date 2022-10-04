#include "group.h"
#include "xmlUtil.h"
#include "string.h"
#include <stdlib.h>

static GroupType_t getGroupType(const char * type) {
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

static TypeDescriptor_t getDescriptor(const char * descriptor) {
	if(strcmp(descriptor, "Optional") == 0) {
		return OPTIONAL;
	} else if(strcmp(descriptor, "Required") == 0) {
		return REQUIRED;
	} else if(strcmp(descriptor, "Recommended") == 0) {
		return RECOMMENDED;
	} else if(strcmp(descriptor, "CouldBeUsable") == 0) {
		return MAYBE_USABLE;
	} else if(strcmp(descriptor, "NotUsable") == 0) {
		return NOT_USABLE;
	} else {
		return -1;
	}
}

void freeGroup(FOModGroup_t * group) {
	if(group->pluginCount == 0) return;
	for(int pluginId = 0; pluginId < group->pluginCount; pluginId++) {
		FOModPlugin_t * plugin = &group->plugins[pluginId];
		if(plugin->fileCount > 0) {
			for(int i = 0; i < plugin->fileCount; i++) {
				free(plugin->files[i].destination);
				free(plugin->files[i].source);
			}
			free(plugin->files);
		}

		if(plugin->flagCount > 0) {
			for(int i = 0; i < plugin->flagCount; i++) {
				free(plugin->flags[i].value);
				free(plugin->flags[i].name);
			}
			free(plugin->flags);
		}

		if(plugin->description != NULL)free(plugin->description);
		if(plugin->image != NULL)free(plugin->image);
		if(plugin->name != NULL)free(plugin->name);
	}
	free(group->plugins);
	group->plugins = NULL;
	group->pluginCount = 0;
}

static int parseConditionFlags(FOModPlugin_t * plugin, xmlNodePtr nodeElement) {
	xmlNodePtr flagNode = nodeElement->children;
	while(flagNode != NULL) {
		if(!validateNode(&flagNode, true, "flag", NULL)) {
			if(plugin->flagCount > 0) {
				free(plugin->flags);
			}
			return EXIT_FAILURE;
		}
		if(flagNode == NULL)continue;

		plugin->flagCount += 1;
		plugin->flags = realloc(plugin->flags, plugin->flagCount * sizeof(FOModFlag_t));

		FOModFlag_t * flag = &plugin->flags[plugin->flagCount - 1];

		flag->name = freeAndDup(xmlGetProp(flagNode, (const xmlChar *) "name"));
		flag->value = freeAndDup(xmlNodeGetContent(flagNode));

		flagNode = flagNode->next;
	}

	return EXIT_SUCCESS;
}

static int parseGroupFiles(FOModPlugin_t * plugin, xmlNodePtr nodeElement) {
	FOModFile_t * files = NULL;
	xmlNodePtr fileNode = nodeElement->children;
	while(fileNode != NULL) {
		if(!validateNode(&fileNode, true, "folder", "file", NULL)) {
			fprintf(stderr, "Unexpected node in files");
			//TODO: free
			return EXIT_FAILURE;
		}

		if(fileNode == NULL)continue;

		plugin->fileCount += 1;

		plugin->files = realloc(plugin->files, (plugin->fileCount + 1) * sizeof(FOModFile_t));
		FOModFile_t * file = &plugin->files[plugin->fileCount - 1];

		file->destination = freeAndDup(xmlGetProp(fileNode, (const xmlChar *) "destination"));
		file->source = freeAndDup(xmlGetProp(fileNode, (const xmlChar *) "source"));

		//TODO: test if it's a number
		xmlChar * priority = xmlGetProp(fileNode, (const xmlChar *) "priority");
		if(priority == NULL) {
			file->priority = 0;
		} else {
			file->priority = atoi((char *)priority);
		}
		xmlFree(priority);
		file->isFolder = xmlStrcmp(fileNode->name, (const xmlChar *) "folder") == 0;
		fileNode = fileNode->next;
	}
	return EXIT_SUCCESS;
}

static int parseNodeElement(FOModPlugin_t * plugin, xmlNodePtr nodeElement) {
	if(xmlStrcmp(nodeElement->name, (const xmlChar *) "description") == 0) {
		plugin->description = freeAndDup(xmlNodeGetContent(nodeElement));
	} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "image") == 0) {
		plugin->image = freeAndDup(xmlGetProp(nodeElement, (const xmlChar *) "path"));
	} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "conditionFlags") == 0) {
		return parseConditionFlags(plugin, nodeElement);
	} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "files") == 0) {
		return parseGroupFiles(plugin, nodeElement);
	} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "typeDescriptor") == 0) {
		xmlNodePtr typeNode = nodeElement->children;
		if(!validateNode(&typeNode, true, "type", NULL)) {
			fprintf(stderr, "Unexpected node in typeDescriptor");
			return EXIT_FAILURE;
		}
		xmlChar * name = xmlGetProp(typeNode, (const xmlChar *) "name");
		plugin->type = getDescriptor((char *) name);
		xmlFree(name);
	}
	return EXIT_SUCCESS;
}

int parseGroup(xmlNodePtr groupNode, FOModGroup_t* group) {
	xmlNodePtr pluginsNode = groupNode->children;
	if(!validateNode(&pluginsNode, true, "plugins", NULL)) {
		return EXIT_FAILURE;
	}


	group->name = freeAndDup(xmlGetProp( groupNode, (const xmlChar *) "name"));

	xmlChar * type = xmlGetProp(groupNode, (const xmlChar *) "type");
	group->type = getGroupType((const char *)type);
	xmlFree(type);

	char * order = (char *) xmlGetProp(pluginsNode, (const xmlChar *) "order");
	group->order = getFOModOrder(order);
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
			if(parseNodeElement(plugin, nodeElement) != EXIT_SUCCESS)
				goto failure;
			nodeElement = nodeElement->next;
		}


		currentPlugin = currentPlugin->next;
	}

	group->plugins = plugins;
	group->pluginCount = pluginCount;
	return EXIT_SUCCESS;

failure:
	//we need to free all of our allocations since we can't expect ou parent function to know what we allocated and what we haven't.
	group->plugins = plugins;
	group->pluginCount = pluginCount;
	freeGroup(group);
	return EXIT_FAILURE;
}
