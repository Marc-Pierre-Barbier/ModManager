#ifndef __GROUP_H__
#define __GROUP_H__

#include <libxml/parser.h>
#include "fomodTypes.h"

typedef enum GroupType_t { ONE_ONLY, ANY, AT_LEAST_ONE, AT_MOST_ONE, ALL } GroupType_t;
typedef enum TypeDescriptor { OPTIONAL, MAYBE_USABLE, NOT_USABLE, REQUIRED, RECOMMENDED } TypeDescriptor_t;

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

//combine group and "plugins"
typedef struct FOModGroup {
	FOModPlugin_t * plugins;
	int pluginCount;
	GroupType_t type;
	char * name;
	FOModOrder_t order;
} FOModGroup_t;

int parseGroup(xmlNodePtr groupNode, FOModGroup_t* group);
void freeGroup(FOModGroup_t * group);


#endif
