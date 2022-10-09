#include "parser.h"
#include "fomodTypes.h"
#include "group.h"
#include "libxml/tree.h"
#include "xmlUtil.h"
#include <stdlib.h>
#include <string.h>

static int parseVisibleNode(xmlNodePtr node, FOModStep_t * step) {
	xmlNodePtr requiredFlagsNode = node->children;

	while (requiredFlagsNode != NULL) {

		if(!xml_validateNode(&requiredFlagsNode, true, "flagDependency", NULL)) {
			//TODO: handle error
			printf("%d\n", __LINE__);
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
			printf("%d\n", __LINE__);
			return EXIT_FAILURE;
		}

		if(group == NULL)break;

		step->groupCount += 1;
		step->groups = realloc(step->groups, step->groupCount * sizeof(fomod_Group_t));
		int status = grp_parseGroup(group, &step->groups[step->groupCount - 1]);

		if(status != EXIT_SUCCESS) {
			//TODO: handle error
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
			printf("%d\n", __LINE__);
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
		printf("%d\n", __LINE__);
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
			printf("%d\n", __LINE__);
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
			printf("%d\n", __LINE__);
			return EXIT_FAILURE;
		}
		xmlNodePtr currentPattern = patterns->children;
		while(currentPattern != NULL) {
			xmlNodePtr patternChild = currentPattern->children;

			if(!xml_validateNode(&patternChild, true, "pattern", NULL)) {
				//TODO: handle error
				printf("%d\n", __LINE__);
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

error_t parser_parseFOMod(const char * fomodFile, FOMod_t* fomod) {
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
		return ERR_FAILURE;
	}


	cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		fprintf(stderr, "emptyDocument");
		xmlFreeDoc(doc);
		return ERR_FAILURE;
	}

	if(xmlStrcmp(cur->name, (const xmlChar *) "config") != 0) {
		fprintf(stderr, "document of the wrong type, root node is '%s' instead of config\n", cur->name);
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
				if(xml_validateNode(&requiredInstallFile, true, "folder", "file", NULL)) {
					//TODO: handle error
					printf("%d\n", __LINE__);
					exit(ERR_FAILURE);
				}

				int size = fomod_countUntilNull(fomod->requiredInstallFiles, sizeof(char **)) + 2;
				fomod->requiredInstallFiles = realloc(fomod->requiredInstallFiles, sizeof(char *) * size);
				//ensure it is null terminated
				fomod->requiredInstallFiles[size - 1] = NULL;
				fomod->requiredInstallFiles[size - 2] = xml_freeAndDup(xmlGetProp(requiredInstallFile, (const xmlChar *)"source"));

				requiredInstallFile = cur->children;
			}
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"installSteps") == 0) {
			if(fomod->steps != NULL) {
				fprintf(stderr, "Multiple 'installSteps' tags");
				//TODO: handle error
				return ERR_FAILURE;
			}

			xmlChar * stepOrder = xmlGetProp(cur, (xmlChar *)"order");
			fomod->stepOrder = fomod_getOrder((char *)stepOrder);
			xmlFree(stepOrder);

			int stepCount = 0;
			FOModStep_t * steps = parseInstallSteps(cur, &stepCount);

			if(steps == NULL) {
				fprintf(stderr, "Failed to parse the install steps\n");

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

	xmlFreeDoc(doc);
	return ERR_SUCCESS;
}
