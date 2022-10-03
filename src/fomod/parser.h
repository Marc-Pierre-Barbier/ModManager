#ifndef __FOMOD_PARSER_H__
#define __FOMOD_PARSER_H__


#include "group.h"

//combine installStep and optionalFileGroups
typedef struct FOModStep {
	FOModOrder_t optionOrder;
	FOModGroup_t * groups;
	int groupCount;
	char * name;
	FOModFlag_t * requiredFlags;
	int flagCount;
} FOModStep_t;

typedef struct FOMod {
	char * moduleName;
	char * moduleImage;
	char ** requiredInstallFiles;
	FOModOrder_t stepOrder;
	FOModStep_t * steps;
	int stepCount;
	FOModCondFile_t * condFiles;
	int condFilesCount;
} FOMod_t;

/**
 * @brief parse a fomod xml file (found inside the mod folder /fomod/moduleconfig.xml (everything should be lowercase))
 *
 * @param fomodFile path to the moduleconfig.xml
 * @param fomod pointer to a new FOMod_t
 * @return error code.
 */
int parseFOMod(const char * fomodFile, FOMod_t* fomod);

/**
 * @brief Free content of a fomod file.
 * @param fomod
 */
void freeFOMod(FOMod_t * fomod);

#endif
