#include <fomod.h>
#include "group.h"
#include <libxml/tree.h>
#include "xmlUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parseVisibleNode(xmlNodePtr node, FOModStep_t * step) {
	printf("Warning unsuported function used\n");
	return EXIT_SUCCESS;
/*
this code in one node deeper than it should
	xmlNodePtr requiredFlagsNode = node->children;

	while (requiredFlagsNode != NULL) {

		if(!xml_validateNode(&requiredFlagsNode, true, "flagDependency", NULL)) {
			//TODO: handle error
			printf("error in parser.c: %d\n", __LINE__);
			return EXIT_FAILURE;
		}

		if(requiredFlagsNode == NULL)break;

		step->flagCount += 1;
		step->requiredFlags = realloc(step->requiredFlags, step->flagCount * sizeof(fomod_Flag_t));
		fomod_Flag_t * flag = &(step->requiredFlags[step->flagCount - 1]);
		flag->name = xml_freeAndDup(xmlGetProp(requiredFlagsNode, (const xmlChar *) "flag"));
		flag->value = xml_freeAndDup(xmlGetProp(requiredFlagsNode, (const xmlChar *) "value"));

		requiredFlagsNode = requiredFlagsNode->next;
	}
*/
	return EXIT_SUCCESS;
}

static int parseOptionalFileGroup(xmlNodePtr node, FOModStep_t * step) {
	xmlChar * optionOrder = xmlGetProp(node, (const xmlChar *)"order");
	step->optionOrder = fomod_getOrder((char *)optionOrder);
	xmlFree(optionOrder);
	xmlNodePtr group = node->children;
	while(group != NULL) {
		if(!xml_validateNode(&group, true, "group", NULL)) {
			//TODO: handle error
			printf("parsing error: parser.c: %d\n", __LINE__);
			return EXIT_FAILURE;
		}

		if(group == NULL)break;

		step->groupCount += 1;
		step->groups = realloc(step->groups, step->groupCount * sizeof(fomod_Group_t));
		int status = grp_parseGroup(group, &step->groups[step->groupCount - 1]);

		if(status != EXIT_SUCCESS) {
			//TODO: handle error
			printf("Parsing error\n");
			return EXIT_FAILURE;
		}

		group = group->next;
	}

	return EXIT_SUCCESS;
}

static FOModStep_t * parseInstallSteps(xmlNodePtr installStepsNode, int * stepCount) {
	FOModStep_t * steps = NULL;
	*stepCount = 0;

	xmlNodePtr stepNode = installStepsNode->children;
	while(stepNode != NULL) {
		//skipping the text node
		if(!xml_validateNode(&stepNode, true, "installStep", NULL)) {
			//TODO: handle error
			printf("parsing error: parser.c: %d\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		if(stepNode == NULL)break;

		*stepCount += 1;
		steps = realloc(steps, *stepCount * sizeof(FOModStep_t));

		FOModStep_t * step = &steps[*stepCount - 1];
		step->name = xml_freeAndDup(xmlGetProp(stepNode, (const xmlChar *)"name"));
		step->requiredFlags = NULL;
		step->flagCount = 0;
		step->groupCount = 0;
		step->groups = NULL;

		xmlNodePtr stepchildren = stepNode->children;

		while(stepchildren != NULL) {
			if(xmlStrcmp(stepchildren->name, (const xmlChar *) "visible") == 0) {
				parseVisibleNode(stepchildren, step);
			} else if(xmlStrcmp(stepchildren->name, (const xmlChar *) "optionalFileGroups") == 0) {
				parseOptionalFileGroup(stepchildren, step);
			}
			stepchildren = stepchildren->next;
		}

		stepNode = stepNode->next;
	}
	return steps;
}

static int parseDependencies(xmlNodePtr node, fomod_CondFile_t * condFile) {
	xmlNodePtr flagNode = node->children;

	if(!xml_validateNode(&flagNode, true, "flagDependency", NULL)) {
		//TODO: handle error
		printf("parsing error: parser.c: %d\n", __LINE__);
		return EXIT_FAILURE;
	}

	while(flagNode != NULL) {
		condFile->flagCount += 1;
		condFile->requiredFlags = realloc(condFile->requiredFlags, condFile->flagCount * sizeof(fomod_Flag_t));
		fomod_Flag_t * flag = &(condFile->requiredFlags[condFile->flagCount - 1]);
		flag->name = xml_freeAndDup(xmlGetProp(flagNode, (const xmlChar *) "flag"));
		flag->value = xml_freeAndDup(xmlGetProp(flagNode, (const xmlChar *) "value"));

		flagNode = flagNode->next;
	}
	return EXIT_SUCCESS;
}

static int parseFiles(xmlNodePtr node, fomod_CondFile_t * condFile) {
	xmlNodePtr filesNode = node->children;


	while(filesNode != NULL) {
		if(!xml_validateNode(&filesNode, true, "folder", "file", NULL)) {
			//TODO: handle error
			printf("parsing error: parser.c: %d\n", __LINE__);
			return EXIT_FAILURE;
		}

		condFile->fileCount += 1;
		condFile->files = realloc(condFile->files, condFile->fileCount * sizeof(fomod_File_t));
		fomod_File_t * flag = &(condFile->files[condFile->fileCount - 1]);
		flag->source = xml_freeAndDup(xmlGetProp(filesNode, (const xmlChar *) "source"));
		flag->destination = xml_freeAndDup(xmlGetProp(filesNode, (const xmlChar *) "destination"));
		flag->priority = 0;
		flag->isFolder = xmlStrcmp(filesNode->name, (xmlChar *) "folder") == 0;

		filesNode = filesNode->next;
	}

	return EXIT_SUCCESS;
}

static int parseConditionalInstalls(xmlNodePtr node, FOMod_t * fomod) {
	xmlNodePtr patterns = node->children;
	if(patterns != NULL) {
		if(!xml_validateNode(&patterns, true, "patterns", NULL)) {
			//TODO: handle error
			printf("parsing error: parser.c: %d\n", __LINE__);
			return EXIT_FAILURE;
		}
		xmlNodePtr currentPattern = patterns->children;
		while(currentPattern != NULL) {
			xmlNodePtr patternChild = currentPattern->children;

			if(!xml_validateNode(&patternChild, true, "pattern", NULL)) {
				//TODO: handle error
				printf("parsing error: parser.c: %d\n", __LINE__);
				return EXIT_FAILURE;
			}

			while(patternChild != NULL) {
				fomod->condFilesCount += 1;
				fomod->condFiles = realloc(fomod->condFiles, fomod->condFilesCount * sizeof(fomod_CondFile_t));
				fomod_CondFile_t * condFile = &(fomod->condFiles[fomod->condFilesCount - 1]);

				condFile->fileCount = 0;
				condFile->files = NULL;
				condFile->flagCount = 0;
				condFile->requiredFlags = NULL;

				if(xmlStrcmp(patternChild->name, (xmlChar *)"dependencies") == 0) {
					if(parseDependencies(patternChild, condFile) != EXIT_SUCCESS) {
						//TODO: handle error
						return EXIT_FAILURE;
					}
				} else if(xmlStrcmp(patternChild->name, (xmlChar *)"files") == 0) {
					if(parseFiles(patternChild, condFile) != EXIT_SUCCESS) {
						return EXIT_FAILURE;
					}
				}
				patternChild = patternChild->next;
			}
			currentPattern = currentPattern->next;
		}
	}
	return EXIT_SUCCESS;
}

static int stepCmpAsc(const void * stepA, const void * stepB) {
	const FOModStep_t * step1 = (const FOModStep_t *)stepA;
	const FOModStep_t * step2 = (const FOModStep_t *)stepB;
	return strcmp(step1->name, step2->name);
}

static int stepCmpDesc(const void * stepA, const void * stepB) {
	const FOModStep_t * step1 = (const FOModStep_t *)stepA;
	const FOModStep_t * step2 = (const FOModStep_t *)stepB;
	return 1 - strcmp(step1->name, step2->name);
}

static void fomod_sortSteps(FOMod_t * fomod) {
	switch(fomod->stepOrder) {
	case ASC:
		qsort(fomod->steps, fomod->stepCount, sizeof(*fomod->steps), stepCmpAsc);
		break;
	case DESC:
		qsort(fomod->steps, fomod->stepCount, sizeof(*fomod->steps), stepCmpDesc);
		break;
	case ORD:
		//ord mean that we keep the curent order, so no need to sort anything.
		break;
	}
}

static int fomod_groupCmpAsc(const void * stepA, const void * stepB) {
	const fomod_Group_t * step1 = (const fomod_Group_t *)stepA;
	const fomod_Group_t * step2 = (const fomod_Group_t *)stepB;
	return strcmp(step1->name, step2->name);
}

static int fomod_groupCmpDesc(const void * stepA, const void * stepB) {
	const fomod_Group_t * step1 = (const fomod_Group_t *)stepA;
	const fomod_Group_t * step2 = (const fomod_Group_t *)stepB;
	return 1 - strcmp(step1->name, step2->name);
}

static void fomod_sortGroup(fomod_Group_t * group) {
	switch(group->order) {
	case ASC:
		qsort(group->plugins, group->pluginCount, sizeof(*group->plugins), fomod_groupCmpAsc);
		break;
	case DESC:
		qsort(group->plugins, group->pluginCount, sizeof(*group->plugins), fomod_groupCmpDesc);
		break;
	case ORD:
		//ord mean that we keep the curent order, so no need to sort anything.
		break;
	}
}

error_t fomod_parse(GFile * fomod_file, FOMod_t* fomod) {
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
			fomod->moduleName = (char *)cur->content;
		} else if(xmlStrcmp(cur->name, (const xmlChar *) "moduleImage") == 0) {
			fomod->moduleImage = xml_freeAndDup(xmlGetProp(cur, (const xmlChar *)"path"));
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"requiredInstallFiles") == 0) {
			//TODO: support non empty destination.
			xmlNodePtr requiredInstallFile = cur->children;
			while(requiredInstallFile != NULL) {
				if(!xml_validateNode(&requiredInstallFile, true, "folder", "file", NULL)) {
					//TODO: handle error
					printf("parsing error: parser.c: %d when parsing line: %d node: %s\n", __LINE__, requiredInstallFile->line, requiredInstallFile->name);
					exit(ERR_FAILURE);
				}
				if(requiredInstallFile == NULL) break;

				int size = fomod_countUntilNull(fomod->requiredInstallFiles) + 2;

				fomod->requiredInstallFiles = realloc(fomod->requiredInstallFiles, sizeof(char *) * size);
				//ensure it is null terminated
				fomod->requiredInstallFiles[size - 1] = NULL;
				fomod->requiredInstallFiles[size - 2] = xml_freeAndDup(xmlGetProp(requiredInstallFile, (const xmlChar *)"source"));
				requiredInstallFile = requiredInstallFile->next;
			}
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"installSteps") == 0) {
			if(fomod->steps != NULL) {
				g_error( "Multiple 'installSteps' tags\n");
				//TODO: handle error
				return ERR_FAILURE;
			}

			xmlChar * stepOrder = xmlGetProp(cur, (xmlChar *)"order");
			fomod->stepOrder = fomod_getOrder((char *)stepOrder);
			xmlFree(stepOrder);

			int stepCount = 0;
			FOModStep_t * steps = parseInstallSteps(cur, &stepCount);

			if(steps == NULL) {
				g_error( "Failed to parse the install steps\n");

				//TODO: manage the error properly
				return ERR_FAILURE;
			}

			fomod->steps = steps;
			fomod->stepCount = stepCount;
		} else if(xmlStrcmp(cur->name, (const xmlChar *) "conditionalFileInstalls") == 0) {
			parseConditionalInstalls(cur, fomod);
		}
		cur = cur->next;
	}

	//steps are in a specific order in fomod config. we need to make sure we respected this order
	fomod_sortSteps(fomod);

	//same for the plugins inside the groups
	for(int i = 0; i < fomod->stepCount; i++)
		for(int j = 0; j < fomod->steps[i].groupCount; j++)
			fomod_sortGroup(&(fomod->steps[i].groups[j]));

	xmlFreeDoc(doc);
	return ERR_SUCCESS;
}
