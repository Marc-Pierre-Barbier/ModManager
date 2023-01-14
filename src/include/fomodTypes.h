#ifndef __FOMOD_TYPES_H__
#define __FOMOD_TYPES_H__

#include "stdbool.h"

typedef enum GroupType_t { ONE_ONLY, ANY, AT_LEAST_ONE, AT_MOST_ONE, ALL } GroupType_t;
typedef enum TypeDescriptor { OPTIONAL, MAYBE_USABLE, NOT_USABLE, REQUIRED, RECOMMENDED } TypeDescriptor_t;
typedef enum FOModOrder { ASC, DESC, ORD } fomod_Order_t;

typedef struct fomod_Flag {
	char * name;
	char * value;
} fomod_Flag_t;

typedef struct fomod_File {
	char * source;
	char * destination;
	int priority;
	bool isFolder;
} fomod_File_t;

typedef struct fomod_CondFile {
	fomod_Flag_t * requiredFlags;
	unsigned int flagCount;
	fomod_File_t * files;
	unsigned int fileCount;
} fomod_CondFile_t;

typedef struct fomod_Plugin {
	char * description;
	char * image;
	fomod_Flag_t * flags;
	int flagCount;
	fomod_File_t * files;
	int fileCount;
	TypeDescriptor_t type;
	char * name;
} fomod_Plugin_t;

//combine group and "plugins"
typedef struct fomod_Group {
	fomod_Plugin_t * plugins;
	int pluginCount;
	GroupType_t type;
	char * name;
	fomod_Order_t order;
} fomod_Group_t;

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


#endif
