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
	FomodFlag_t * required_flags;
	unsigned int flag_count;
	FomodFile_t * files;
	unsigned int file_count;
} FomodCondFile_t;

typedef struct FomodPlugin {
	char * description;
	char * image;
	FomodFlag_t * flags;
	int flag_count;
	FomodFile_t * files;
	int file_count;
	TypeDescriptor_t type;
	char * name;
} FomodPlugin_t;

//combine group and "plugins"
typedef struct fomodGroup {
	FomodPlugin_t * plugins;
	int plugin_count;
	GroupType_t type;
	char * name;
	fomod_Order_t order;
} FomodGroup_t;

//combine installStep and optionalFileGroups
typedef struct FomodStep {
	fomod_Order_t option_order;
	FomodGroup_t * groups;
	int group_count;
	char * name;
	FomodFlag_t * required_flags;
	int flag_count;
} FomodStep_t;

typedef struct Fomod {
	char * module_name;
	char * module_image;
	char ** required_install_files;
	fomod_Order_t step_order;
	FomodStep_t * steps;
	int step_count;
	FomodCondFile_t * cond_files;
	int cond_files_count;
} Fomod_t;


#endif
