#include <fomod.h>
#include "group.h"
#include <libxml/tree.h>
#include "xmlUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_visible_node(xmlNodePtr node, FomodStep_t * step) {
	printf("Warning unsuported function used\n");
	return EXIT_SUCCESS;
/*
this code in one node deeper than it should
	xmlNodePtr required_flagsNode = node->children;

	while (required_flagsNode != NULL) {

		if(!xml_validateNode(&required_flagsNode, true, "flagDependency", NULL)) {
			//TODO: handle error
			printf("error in parser.c: %d\n", __LINE__);
			return EXIT_FAILURE;
		}

		if(required_flagsNode == NULL)break;

		step->flag_count += 1;
		step->required_flags = realloc(step->required_flags, step->flag_count * sizeof(fomod_Flag_t));
		fomod_Flag_t * flag = &(step->required_flags[step->flag_count - 1]);
		flag->name = xml_freeAndDup(xmlGetProp(required_flagsNode, (const xmlChar *) "flag"));
		flag->value = xml_freeAndDup(xmlGetProp(required_flagsNode, (const xmlChar *) "value"));

		required_flagsNode = required_flagsNode->next;
	}
*/
	return EXIT_SUCCESS;
}

static int parse_optional_file_group(xmlNodePtr node, FomodStep_t * step) {
	xmlChar * option_order = xmlGetProp(node, (const xmlChar *)"order");
	step->option_order = fomod_get_order((char *)option_order);
	xmlFree(option_order);
	xmlNodePtr group = node->children;
	while(group != NULL) {
		if(!xml_validate_node(&group, true, "group", NULL)) {
			//TODO: handle error
			printf("parsing error: parser.c: %d\n", __LINE__);
			return EXIT_FAILURE;
		}

		if(group == NULL)break;

		step->group_count += 1;
		step->groups = realloc(step->groups, step->group_count * sizeof(FomodGroup_t));
		int status = grp_parse_group(group, &step->groups[step->group_count - 1]);

		if(status != EXIT_SUCCESS) {
			//TODO: handle error
			printf("Parsing error\n");
			return EXIT_FAILURE;
		}

		group = group->next;
	}

	return EXIT_SUCCESS;
}

static FomodStep_t * parse_install_steps(xmlNodePtr install_steps_node, int * step_count) {
	FomodStep_t * steps = NULL;
	*step_count = 0;

	xmlNodePtr step_node = install_steps_node->children;
	while(step_node != NULL) {
		//skipping the text node
		if(!xml_validate_node(&step_node, true, "installStep", NULL)) {
			//TODO: handle error
			printf("parsing error: parser.c: %d\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		if(step_node == NULL)break;

		*step_count += 1;
		steps = realloc(steps, *step_count * sizeof(FomodStep_t));

		FomodStep_t * step = &steps[*step_count - 1];
		step->name = xml_free_and_dup(xmlGetProp(step_node, (const xmlChar *)"name"));
		step->required_flags = NULL;
		step->flag_count = 0;
		step->group_count = 0;
		step->groups = NULL;

		xmlNodePtr step_childrens = step_node->children;

		while(step_childrens != NULL) {
			if(xmlStrcmp(step_childrens->name, (const xmlChar *) "visible") == 0) {
				parse_visible_node(step_childrens, step);
			} else if(xmlStrcmp(step_childrens->name, (const xmlChar *) "optionalFileGroups") == 0) {
				parse_optional_file_group(step_childrens, step);
			}
			step_childrens = step_childrens->next;
		}

		step_node = step_node->next;
	}
	return steps;
}

static int parseDependencies(xmlNodePtr node, FomodCondFile_t * cond_file) {
	xmlNodePtr flag_node = node->children;

	if(!xml_validate_node(&flag_node, true, "flagDependency", NULL)) {
		//TODO: handle error
		printf("parsing error: parser.c: %d\n", __LINE__);
		return EXIT_FAILURE;
	}

	while(flag_node != NULL) {
		cond_file->flag_count += 1;
		cond_file->required_flags = realloc(cond_file->required_flags, cond_file->flag_count * sizeof(FomodFlag_t));
		FomodFlag_t * flag = &(cond_file->required_flags[cond_file->flag_count - 1]);
		flag->name = xml_free_and_dup(xmlGetProp(flag_node, (const xmlChar *) "flag"));
		flag->value = xml_free_and_dup(xmlGetProp(flag_node, (const xmlChar *) "value"));

		flag_node = flag_node->next;
	}
	return EXIT_SUCCESS;
}

static int parseFiles(xmlNodePtr node, FomodCondFile_t * cond_file) {
	xmlNodePtr filesNode = node->children;


	while(filesNode != NULL) {
		if(!xml_validate_node(&filesNode, true, "folder", "file", NULL)) {
			//TODO: handle error
			printf("parsing error: parser.c: %d\n", __LINE__);
			return EXIT_FAILURE;
		}

		cond_file->file_count += 1;
		cond_file->files = realloc(cond_file->files, cond_file->file_count * sizeof(FomodFile_t));
		FomodFile_t * flag = &(cond_file->files[cond_file->file_count - 1]);
		flag->source = xml_free_and_dup(xmlGetProp(filesNode, (const xmlChar *) "source"));
		flag->destination = xml_free_and_dup(xmlGetProp(filesNode, (const xmlChar *) "destination"));
		flag->priority = 0;
		flag->isFolder = xmlStrcmp(filesNode->name, (xmlChar *) "folder") == 0;

		filesNode = filesNode->next;
	}

	return EXIT_SUCCESS;
}

static int parseConditionalInstalls(xmlNodePtr node, Fomod_t * fomod) {
	xmlNodePtr patterns = node->children;
	if(patterns != NULL) {
		if(!xml_validate_node(&patterns, true, "patterns", NULL)) {
			//TODO: handle error
			printf("parsing error: parser.c: %d\n", __LINE__);
			return EXIT_FAILURE;
		}
		xmlNodePtr current_pattern = patterns->children;
		while(current_pattern != NULL) {
			xmlNodePtr pattern_child = current_pattern->children;

			if(!xml_validate_node(&pattern_child, true, "pattern", NULL)) {
				//TODO: handle error
				printf("parsing error: parser.c: %d\n", __LINE__);
				return EXIT_FAILURE;
			}

			while(pattern_child != NULL) {
				fomod->cond_files_count += 1;
				fomod->cond_files = realloc(fomod->cond_files, fomod->cond_files_count * sizeof(FomodCondFile_t));
				FomodCondFile_t * cond_file = &(fomod->cond_files[fomod->cond_files_count - 1]);

				cond_file->file_count = 0;
				cond_file->files = NULL;
				cond_file->flag_count = 0;
				cond_file->required_flags = NULL;

				if(xmlStrcmp(pattern_child->name, (xmlChar *)"dependencies") == 0) {
					if(parseDependencies(pattern_child, cond_file) != EXIT_SUCCESS) {
						//TODO: handle error
						return EXIT_FAILURE;
					}
				} else if(xmlStrcmp(pattern_child->name, (xmlChar *)"files") == 0) {
					if(parseFiles(pattern_child, cond_file) != EXIT_SUCCESS) {
						return EXIT_FAILURE;
					}
				}
				pattern_child = pattern_child->next;
			}
			current_pattern = current_pattern->next;
		}
	}
	return EXIT_SUCCESS;
}

static int step_cmp_asc(const void * stepA, const void * stepB) {
	const FomodStep_t * step1 = (const FomodStep_t *)stepA;
	const FomodStep_t * step2 = (const FomodStep_t *)stepB;
	return strcmp(step1->name, step2->name);
}

static int step_cmp_desc(const void * stepA, const void * stepB) {
	const FomodStep_t * step1 = (const FomodStep_t *)stepA;
	const FomodStep_t * step2 = (const FomodStep_t *)stepB;
	return 1 - strcmp(step1->name, step2->name);
}

static void fomod_sortSteps(Fomod_t * fomod) {
	switch(fomod->step_order) {
	case ASC:
		qsort(fomod->steps, fomod->step_count, sizeof(*fomod->steps), step_cmp_asc);
		break;
	case DESC:
		qsort(fomod->steps, fomod->step_count, sizeof(*fomod->steps), step_cmp_desc);
		break;
	case ORD:
		//ord mean that we keep the curent order, so no need to sort anything.
		break;
	}
}

static int fomod_group_cmp_asc(const void * stepA, const void * stepB) {
	const FomodGroup_t * step1 = (const FomodGroup_t *)stepA;
	const FomodGroup_t * step2 = (const FomodGroup_t *)stepB;
	return strcmp(step1->name, step2->name);
}

static int fomod_group_cmp_desc(const void * stepA, const void * stepB) {
	const FomodGroup_t * step1 = (const FomodGroup_t *)stepA;
	const FomodGroup_t * step2 = (const FomodGroup_t *)stepB;
	return 1 - strcmp(step1->name, step2->name);
}

static void fomod_sortGroup(FomodGroup_t * group) {
	switch(group->order) {
	case ASC:
		qsort(group->plugins, group->plugin_count, sizeof(*group->plugins), fomod_group_cmp_asc);
		break;
	case DESC:
		qsort(group->plugins, group->plugin_count, sizeof(*group->plugins), fomod_group_cmp_desc);
		break;
	case ORD:
		//ord mean that we keep the curent order, so no need to sort anything.
		break;
	}
}

error_t fomod_parse(GFile * fomod_file, Fomod_t* fomod) {
	xmlDocPtr doc;
	xmlNodePtr cur;

	fomod->cond_files = NULL;
	fomod->cond_files_count = 0;
	fomod->required_install_files = NULL;
	fomod->step_count = 0;
	fomod->step_order = 0;
	fomod->steps = NULL;
	fomod->module_image = NULL;
	fomod->module_name = NULL;

	g_autofree char * fomod_file_path = g_file_get_path(fomod_file);
	doc = xmlParseFile(fomod_file_path);

	if(doc == NULL) {
		g_error( "Document is not a valid xml file\n");
		return ERR_FAILURE;
	}


	cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		g_error( "emptyDocument\n");
		xmlFreeDoc(doc);
		return ERR_FAILURE;
	}

	if(xmlStrcmp(cur->name, (const xmlChar *) "config") != 0) {
		g_error( "document of the wrong type, root node is '%s' instead of config\n", cur->name);
		xmlFreeDoc(doc);
		return ERR_FAILURE;
	}

	cur = cur->children;
	while(cur != NULL) {
		if(xmlStrcmp(cur->name, (const xmlChar *) "moduleName") == 0) {
			//might cause some issues. when will c finally support utf-8
			fomod->module_name = (char *)cur->content;
		} else if(xmlStrcmp(cur->name, (const xmlChar *) "moduleImage") == 0) {
			fomod->module_image = xml_free_and_dup(xmlGetProp(cur, (const xmlChar *)"path"));
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"requiredInstallFiles") == 0) {
			//TODO: support non empty destination.
			xmlNodePtr required_install_file = cur->children;
			while(required_install_file != NULL) {
				if(!xml_validate_node(&required_install_file, true, "folder", "file", NULL)) {
					//TODO: handle error
					printf("parsing error: parser.c: %d when parsing line: %d node: %s\n", __LINE__, required_install_file->line, required_install_file->name);
					exit(ERR_FAILURE);
				}
				if(required_install_file == NULL) break;

				int size = fomod_count_until_null(fomod->required_install_files) + 2;

				fomod->required_install_files = realloc(fomod->required_install_files, sizeof(char *) * size);
				//ensure it is null terminated
				fomod->required_install_files[size - 1] = NULL;
				fomod->required_install_files[size - 2] = xml_free_and_dup(xmlGetProp(required_install_file, (const xmlChar *)"source"));
				required_install_file = required_install_file->next;
			}
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"installSteps") == 0) {
			if(fomod->steps != NULL) {
				g_error( "Multiple 'installSteps' tags\n");
				//TODO: handle error
				return ERR_FAILURE;
			}

			xmlChar * step_order = xmlGetProp(cur, (xmlChar *)"order");
			fomod->step_order = fomod_get_order((char *)step_order);
			xmlFree(step_order);

			int stepCount = 0;
			FomodStep_t * steps = parse_install_steps(cur, &stepCount);

			if(steps == NULL) {
				g_error( "Failed to parse the install steps\n");

				//TODO: manage the error properly
				return ERR_FAILURE;
			}

			fomod->steps = steps;
			fomod->step_count = stepCount;
		} else if(xmlStrcmp(cur->name, (const xmlChar *) "conditionalFileInstalls") == 0) {
			parseConditionalInstalls(cur, fomod);
		}
		cur = cur->next;
	}

	//steps are in a specific order in fomod config. we need to make sure we respected this order
	fomod_sortSteps(fomod);

	//same for the plugins inside the groups
	for(int i = 0; i < fomod->step_count; i++)
		for(int j = 0; j < fomod->steps[i].group_count; j++)
			fomod_sortGroup(&(fomod->steps[i].groups[j]));

	xmlFreeDoc(doc);
	return ERR_SUCCESS;
}
