#include "group.h"
#include "fomodTypes.h"
#include "libxml/tree.h"
#include "libxml/xmlstring.h"
#include "xmlUtil.h"
#include "string.h"
#include <stdlib.h>

static GroupType_t get_group_type(const char * type) {
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

static TypeDescriptor_t get_descriptor(const char * descriptor) {
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

void grp_free_group(FomodGroup_t * group){
	free(group->name);
	if(group->plugin_count == 0) return;
	for(int pluginId = 0; pluginId < group->plugin_count; pluginId++) {
		FomodPlugin_t * plugin = &group->plugins[pluginId];
		if(plugin->file_count > 0) {
			for(int i = 0; i < plugin->file_count; i++) {
				free(plugin->files[i].destination);
				free(plugin->files[i].source);
			}
			free(plugin->files);
		}

		if(plugin->flag_count > 0) {
			for(int i = 0; i < plugin->flag_count; i++) {
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
	group->plugin_count = 0;
}

static int parse_condition_flags(FomodPlugin_t * plugin, xmlNodePtr nodeElement) {
	xmlNodePtr flag_node = nodeElement->children;
	while(flag_node != NULL) {
		if(!xml_validate_node(&flag_node, true, "flag", NULL)) {
			if(plugin->flag_count > 0) {
				free(plugin->flags);
			}
			return EXIT_FAILURE;
		}
		if(flag_node == NULL)continue;

		plugin->flag_count += 1;
		plugin->flags = realloc(plugin->flags, plugin->flag_count * sizeof(FomodFlag_t));

		FomodFlag_t * flag = &plugin->flags[plugin->flag_count - 1];

		flag->name = xml_free_and_dup(xmlGetProp(flag_node, (const xmlChar *) "name"));
		flag->value = xml_free_and_dup(xmlNodeGetContent(flag_node));

		flag_node = flag_node->next;
	}

	return EXIT_SUCCESS;
}

static int parseGroupFiles(FomodPlugin_t * plugin, xmlNodePtr node_element) {
	xmlNodePtr file_node = node_element->children;
	while(file_node != NULL) {
		if(!xml_validate_node(&file_node, true, "folder", "file", NULL)) {
			g_error( "Unexpected node in files\n");
			//TODO: free
			return EXIT_FAILURE;
		}

		if(file_node == NULL)continue;

		plugin->file_count += 1;

		plugin->files = realloc(plugin->files, (plugin->file_count + 1) * sizeof(FomodFile_t));
		FomodFile_t * file = &plugin->files[plugin->file_count - 1];

		file->destination = xml_free_and_dup(xmlGetProp(file_node, (const xmlChar *) "destination"));
		file->source = xml_free_and_dup(xmlGetProp(file_node, (const xmlChar *) "source"));

		//TODO: test if it's a number
		xmlChar * priority = xmlGetProp(file_node, (const xmlChar *) "priority");
		if(priority == NULL) {
			file->priority = 0;
		} else {
			file->priority = atoi((char *)priority);
		}
		xmlFree(priority);
		file->isFolder = xmlStrcmp(file_node->name, (const xmlChar *) "folder") == 0;
		file_node = file_node->next;
	}
	return EXIT_SUCCESS;
}

static int parseNodeElement(FomodPlugin_t * plugin, xmlNodePtr node_element) {
	if(xmlStrcmp(node_element->name, (const xmlChar *) "description") == 0) {
		plugin->description = xml_free_and_dup(xmlNodeGetContent(node_element));
	} else if(xmlStrcmp(node_element->name, (const xmlChar *) "image") == 0) {
		//TODO: maybe absolute path
		char * path = xml_free_and_dup(xmlGetProp(node_element, (const xmlChar *) "path"));
		xml_fix_path(path);
		//TODO: make it safer.
		plugin->image = g_ascii_strdown(path,strlen(path));
		free(path);
	} else if(xmlStrcmp(node_element->name, (const xmlChar *) "conditionFlags") == 0) {
		return parse_condition_flags(plugin, node_element);
	} else if(xmlStrcmp(node_element->name, (const xmlChar *) "files") == 0) {
		return parseGroupFiles(plugin, node_element);
	} else if(xmlStrcmp(node_element->name, (const xmlChar *) "typeDescriptor") == 0) {
		//WEIRD SHIT
		xmlNodePtr typeNode = node_element->children;
		if(!xml_validate_node(&typeNode, true, "type", "dependencyType", NULL)) {
			g_error( "Unexpected node in typeDescriptor: %s at %d\n", typeNode->name, typeNode->line);
			return EXIT_FAILURE;
		}

		if(xmlStrcmp(node_element->name, (const xmlChar *) "dependencyType")) {
			//TODO: add support for it
			printf("Warning using unsupported functionnality\n");
			plugin->type = OPTIONAL;
		} else {
			xmlChar * name = xmlGetProp(typeNode, (const xmlChar *) "name");
			plugin->type = get_descriptor((char *) name);
			xmlFree(name);
		}

	}
	return EXIT_SUCCESS;
}

int grp_parse_group(xmlNodePtr groupNode, FomodGroup_t* group) {
	xmlNodePtr plugins_node = groupNode->children;
	if(!xml_validate_node(&plugins_node, true, "plugins", NULL)) {
		return EXIT_FAILURE;
	}


	group->name = xml_free_and_dup(xmlGetProp( groupNode, (const xmlChar *) "name"));

	xmlChar * type = xmlGetProp(groupNode, (const xmlChar *) "type");
	group->type = get_group_type((const char *)type);
	xmlFree(type);

	char * order = (char *) xmlGetProp(plugins_node, (const xmlChar *) "order");
	group->order = fomod_get_order(order);
	xmlFree(order);

	FomodPlugin_t * plugins = NULL;
	int plugin_count = 0;

	xmlNodePtr current_plugin = plugins_node->children;
	while(current_plugin != NULL) {
		if(!xml_validate_node(&current_plugin, true, "plugin", NULL)) {
			//TODO handle error;
			printf("group.c: %d\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		if(current_plugin == NULL)continue;


		plugin_count += 1;
		plugins = realloc(plugins, plugin_count * sizeof(FomodPlugin_t));
		FomodPlugin_t * plugin = &plugins[plugin_count - 1];
		//initialise everything to 0 and null pointers
		memset(plugin, 0, sizeof(FomodPlugin_t));

		plugin->name = xml_free_and_dup(xmlGetProp(current_plugin, (const xmlChar *) "name"));

		xmlNodePtr nodeElement = current_plugin->children;
		while(nodeElement != NULL) {
			if(parseNodeElement(plugin, nodeElement) != EXIT_SUCCESS)
				goto failure;
			nodeElement = nodeElement->next;
		}


		current_plugin = current_plugin->next;
	}

	group->plugins = plugins;
	group->plugin_count = plugin_count;
	return EXIT_SUCCESS;

failure:
	//we need to free all of our allocations since we can't expect ou parent function to know what we allocated and what we haven't.
	group->plugins = plugins;
	group->plugin_count = plugin_count;
	grp_free_group(group);
	return EXIT_FAILURE;
}
