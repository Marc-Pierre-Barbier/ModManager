#include "parser.h"
#include "libxml/tree.h"
#include "xmlUtil.h"
#include <stdlib.h>
#include <string.h>

//Maybe integrate this into the rest of the code instead of freeing after the fact
void freeFOMod(FOMod_t * fomod) {
	for(int i = 0; i < fomod->condFilesCount; i++) {
		FOModCondFile_t * condFile = &(fomod->condFiles[i]);
		for(long fileId = 0; fileId < condFile->fileCount; fileId++) {
			free(condFile->files[fileId].destination);
			free(condFile->files[fileId].source);
		}

		for(long flagId = 0; flagId < condFile->flagCount; flagId++) {
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

	int size = countUntilNull(fomod->requiredInstallFiles, sizeof(char **));
	for(int i = 0; i < size; i++) {
		free(fomod->requiredInstallFiles[i]);
	}
	free(fomod->requiredInstallFiles);

	for(int i = 0; i < fomod->stepCount; i++) {
		FOModStep_t * step = &fomod->steps[i];
		for(int groupId = 0; groupId < step->groupCount; groupId++) {
			FOModGroup_t * group = &step->groups[groupId];
			freeGroup(group);
		}
		for(int flagId = 0; flagId < step->flagCount; flagId++) {
			FOModFlag_t * flag = &(step->requiredFlags[flagId]);
			free(flag->name);
			free(flag->value);
		}
		free(step->groups);
		free(step->requiredFlags);
		free(step->name);
	}
	free(fomod->steps);

	//set every counter to zero and every pointer to null
	memset(fomod, 0, sizeof(FOMod_t));
}

static int parseVisibleNode(xmlNodePtr node, FOModStep_t * step) {
	xmlNodePtr requiredFlagsNode = node->children;

	while (requiredFlagsNode != NULL) {

		if(!validateNode(&requiredFlagsNode, true, "flagDependency", NULL)) {
			//TODO: handle error
			printf("%d\n", __LINE__);
			return EXIT_FAILURE;
		}

		if(requiredFlagsNode == NULL)break;

		step->flagCount += 1;
		step->requiredFlags = realloc(step->requiredFlags, step->flagCount * sizeof(FOModFlag_t));
		FOModFlag_t * flag = &(step->requiredFlags[step->flagCount - 1]);
		flag->name = freeAndDup(xmlGetProp(requiredFlagsNode, (const xmlChar *) "flag"));
		flag->value = freeAndDup(xmlGetProp(requiredFlagsNode, (const xmlChar *) "value"));

		requiredFlagsNode = requiredFlagsNode->next;
	}

	return EXIT_SUCCESS;
}

static int parseOptionalFileGroup(xmlNodePtr node, FOModStep_t * step) {
	xmlChar * optionOrder = xmlGetProp(node, (const xmlChar *)"order");
	step->optionOrder = getFOModOrder((char *)optionOrder);
	xmlFree(optionOrder);
	xmlNodePtr group = node->children;
	while(group != NULL) {
		if(!validateNode(&group, true, "group", NULL)) {
			//TODO: handle error
			printf("%d\n", __LINE__);
			return EXIT_FAILURE;
		}

		if(group == NULL)break;

		step->groupCount += 1;
		step->groups = realloc(step->groups, step->groupCount * sizeof(FOModGroup_t));
		int status = parseGroup(group, &step->groups[step->groupCount - 1]);

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

static int parseDependencies(xmlNodePtr node, FOModCondFile_t * condFile) {
	xmlNodePtr flagNode = node->children;

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
	return EXIT_SUCCESS;
}

static int parseFiles(xmlNodePtr node, FOModCondFile_t * condFile) {
	xmlNodePtr filesNode = node->children;


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
		flag->isFolder = xmlStrcmp(filesNode->name, (xmlChar *) "folder") == 0;

		filesNode = filesNode->next;
	}

	return EXIT_SUCCESS;
}

static int parseConditionalInstalls(xmlNodePtr node, FOMod_t * fomod) {
	xmlNodePtr patterns = node->children;
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

				int size = countUntilNull(fomod->requiredInstallFiles, sizeof(char **)) + 2;
				fomod->requiredInstallFiles = realloc(fomod->requiredInstallFiles, sizeof(char *) * size);
				//ensure it is null terminated
				fomod->requiredInstallFiles[size - 1] = NULL;
				fomod->requiredInstallFiles[size - 2] = freeAndDup(xmlGetProp(requiredInstallFile, (const xmlChar *)"source"));

				requiredInstallFile = cur->children;
			}
		} else if(xmlStrcmp(cur->name, (const xmlChar *)"installSteps") == 0) {
			if(fomod->steps != NULL) {
				fprintf(stderr, "Multiple 'installSteps' tags");
				//TODO: handle error
				return EXIT_FAILURE;
			}

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
			parseConditionalInstalls(cur, fomod);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return EXIT_SUCCESS;
}
