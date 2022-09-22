#ifndef __FOMOD_H__
#define __FOMOD_H__

#include <stdbool.h>

typedef enum FOModOrder { ASC, DESC, ORD } FOModOrder_t;
typedef enum GroupType_t { ONE_ONLY, ANY, AT_LEAST_ONE, AT_MOST_ONE, ALL } GroupType_t;
typedef enum TypeDescriptor { OPTIONAL, MAYBE_USABLE, NOT_USABLE, REQUIRED, RECOMMENDED } TypeDescriptor_t;

typedef struct FOModFlag {
	char * name;
	char * value;
} FOModFlag_t;

typedef struct FOModFile {
	char * source;
	char * destination;
	int priority;
	bool isFolder;
} FOModFile_t;

typedef struct FOModPlugin {
	char * description;
	char * image;
	FOModFlag_t * flags;
	int flagCount;
	FOModFile_t * files;
	int fileCount;
	TypeDescriptor_t type;
	char * name;
} FOModPlugin_t;

//combine group and plugins
typedef struct FOModGroup {
	FOModPlugin_t * plugins;
	int pluginCount;
	GroupType_t type;
	char * name;
	FOModOrder_t order;
} FOModGroup_t;

//combine installStep and optionalFileGroups
typedef struct FOModStep {
	FOModOrder_t optionOrder;
	FOModGroup_t * groups;
	char * name;
	FOModFlag_t * requiredFlags;
	int flagCount;
} FOModStep_t;

typedef struct FOModCondFile {
	FOModFlag_t * requiredFlags;
	unsigned int flagCount;
	FOModFile_t * files;
	unsigned int fileCount;
} FOModCondFile_t;

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

void freeFOMod(FOMod_t * fomod);

#endif
