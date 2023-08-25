#ifndef __FOMOD_TYPES_H__
#define __FOMOD_TYPES_H__

#include "stdbool.h"

typedef enum GroupType_t { ONE_ONLY, ANY, AT_LEAST_ONE, AT_MOST_ONE, ALL } GroupType_t;
typedef enum TypeDescriptor { OPTIONAL, MAYBE_USABLE, NOT_USABLE, REQUIRED, RECOMMENDED } TypeDescriptor_t;
typedef enum FOModOrder { ASC, DESC, ORD } fomod_Order_t;

typedef struct FomodFlag {
	char * name;
	char * value;
} FomodFlag_t;

typedef struct fomod_File {
	char * source;
	char * destination;
	int priority;
	bool isFolder;
} FomodFile_t;

typedef struct fomodCondFile {
	FomodFlag_t * requiredFlags;
	unsigned int flagCount;
	FomodFile_t * files;
	unsigned int fileCount;
} FomodCondFile_t;

typedef struct FomodPlugin {
	char * description;
	char * image;
	FomodFlag_t * flags;
	int flagCount;
	FomodFile_t * files;
	int fileCount;
	TypeDescriptor_t type;
	char * name;
} FomodPlugin_t;

//combine group and "plugins"
typedef struct fomodGroup {
	FomodPlugin_t * plugins;
	int pluginCount;
	GroupType_t type;
	char * name;
	fomod_Order_t order;
} fomodGroup_t;

//combine installStep and optionalFileGroups
typedef struct FomodStep {
	fomod_Order_t optionOrder;
	fomodGroup_t * groups;
	int groupCount;
	char * name;
	FomodFlag_t * requiredFlags;
	int flagCount;
} FomodStep_t;

typedef struct Fomod {
	char * moduleName;
	char * moduleImage;
	char ** requiredInstallFiles;
	fomod_Order_t stepOrder;
	FomodStep_t * steps;
	int stepCount;
	FomodCondFile_t * condFiles;
	int condFilesCount;
} Fomod_t;


#endif
