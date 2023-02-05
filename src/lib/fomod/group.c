#include "group.h"
#include "fomodTypes.h"
#include "libxml/tree.h"
#include "libxml/xmlstring.h"
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

void grp_freeGroup(fomod_Group_t * group){
	free(group->name);
	if(group->pluginCount == 0) return;
	for(int pluginId = 0; pluginId < group->pluginCount; pluginId++) {
		fomod_Plugin_t * plugin = &group->plugins[pluginId];
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

static int parseConditionFlags(fomod_Plugin_t * plugin, xmlNodePtr nodeElement) {
	xmlNodePtr flagNode = nodeElement->children;
	while(flagNode != NULL) {
		if(!xml_validateNode(&flagNode, true, "flag", NULL)) {
			if(plugin->flagCount > 0) {
				free(plugin->flags);
			}
			return EXIT_FAILURE;
		}
		if(flagNode == NULL)continue;

		plugin->flagCount += 1;
		plugin->flags = realloc(plugin->flags, plugin->flagCount * sizeof(fomod_Flag_t));

		fomod_Flag_t * flag = &plugin->flags[plugin->flagCount - 1];

		flag->name = xml_freeAndDup(xmlGetProp(flagNode, (const xmlChar *) "name"));
		flag->value = xml_freeAndDup(xmlNodeGetContent(flagNode));

		flagNode = flagNode->next;
	}

	return EXIT_SUCCESS;
}

static int parseGroupFiles(fomod_Plugin_t * plugin, xmlNodePtr nodeElement) {
	xmlNodePtr fileNode = nodeElement->children;
	while(fileNode != NULL) {
		if(!xml_validateNode(&fileNode, true, "folder", "file", NULL)) {
			fprintf(stderr, "Unexpected node in files\n");
			//TODO: free
			return EXIT_FAILURE;
		}

		if(fileNode == NULL)continue;

		plugin->fileCount += 1;

		plugin->files = realloc(plugin->files, (plugin->fileCount + 1) * sizeof(fomod_File_t));
		fomod_File_t * file = &plugin->files[plugin->fileCount - 1];

		file->destination = xml_freeAndDup(xmlGetProp(fileNode, (const xmlChar *) "destination"));
		file->source = xml_freeAndDup(xmlGetProp(fileNode, (const xmlChar *) "source"));

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

static int parseNodeElement(fomod_Plugin_t * plugin, xmlNodePtr nodeElement) {
	if(xmlStrcmp(nodeElement->name, (const xmlChar *) "description") == 0) {
		plugin->description = xml_freeAndDup(xmlNodeGetContent(nodeElement));
	} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "image") == 0) {
		plugin->image = xml_freeAndDup(xmlGetProp(nodeElement, (const xmlChar *) "path"));
	} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "conditionFlags") == 0) {
		return parseConditionFlags(plugin, nodeElement);
	} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "files") == 0) {
		return parseGroupFiles(plugin, nodeElement);
	} else if(xmlStrcmp(nodeElement->name, (const xmlChar *) "typeDescriptor") == 0) {
		//WEIRD SHIT
		xmlNodePtr typeNode = nodeElement->children;
		if(!xml_validateNode(&typeNode, true, "type", "dependencyType", NULL)) {
			fprintf(stderr, "Unexpected node in typeDescriptor: %s at %d\n", typeNode->name, typeNode->line);
			return EXIT_FAILURE;
		}

		if(xmlStrcmp(nodeElement->name, (const xmlChar *) "dependencyType")) {
			//TODO: add support for it
			printf("Warning using unsupported functionnality\n");
			plugin->type = OPTIONAL;
		} else {
			xmlChar * name = xmlGetProp(typeNode, (const xmlChar *) "name");
			plugin->type = getDescriptor((char *) name);
			xmlFree(name);
		}

	}
	return EXIT_SUCCESS;
}

int grp_parseGroup(xmlNodePtr groupNode, fomod_Group_t* group) {
	xmlNodePtr pluginsNode = groupNode->children;
	if(!xml_validateNode(&pluginsNode, true, "plugins", NULL)) {
		return EXIT_FAILURE;
	}


	group->name = xml_freeAndDup(xmlGetProp( groupNode, (const xmlChar *) "name"));

	xmlChar * type = xmlGetProp(groupNode, (const xmlChar *) "type");
	group->type = getGroupType((const char *)type);
	xmlFree(type);

	char * order = (char *) xmlGetProp(pluginsNode, (const xmlChar *) "order");
	group->order = fomod_getOrder(order);
	xmlFree(order);

	fomod_Plugin_t * plugins = NULL;
	int pluginCount = 0;

	xmlNodePtr currentPlugin = pluginsNode->children;
	while(currentPlugin != NULL) {
		if(!xml_validateNode(&currentPlugin, true, "plugin", NULL)) {
			//TODO handle error;
			printf("group.c: %d\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		if(currentPlugin == NULL)continue;


		pluginCount += 1;
		plugins = realloc(plugins, pluginCount * sizeof(fomod_Plugin_t));
		fomod_Plugin_t * plugin = &plugins[pluginCount - 1];
		//initialise everything to 0 and null pointers
		memset(plugin, 0, sizeof(fomod_Plugin_t));

		plugin->name = xml_freeAndDup(xmlGetProp(currentPlugin, (const xmlChar *) "name"));

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
	grp_freeGroup(group);
	return EXIT_FAILURE;
}
