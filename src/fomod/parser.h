#ifndef __FOMOD_PARSER_H__
#define __FOMOD_PARSER_H__


#include "fomodTypes.h"
#include "group.h"
#include "../main.h"

//combine installStep and optionalFileGroups
typedef struct FOModStep {
	fomod_Order_t optionOrder;
	fomod_Group_t * groups;
	int groupCount;
	char * name;
	fomod_Flag_t * requiredFlags;
	int flagCount;
} FOModStep_t;

typedef struct FOMod {
	char * moduleName;
	char * moduleImage;
	char ** requiredInstallFiles;
	fomod_Order_t stepOrder;
	FOModStep_t * steps;
	int stepCount;
	fomod_CondFile_t * condFiles;
	int condFilesCount;
} FOMod_t;

/**
 * @brief parse a fomod xml file (found inside the mod folder /fomod/moduleconfig.xml (everything should be lowercase))
 *
 * @param fomodFile path to the moduleconfig.xml
 * @param fomod pointer to a new FOMod_t
 */
error_t parser_parseFOMod(const char * fomodFile, FOMod_t* fomod);

#endif
