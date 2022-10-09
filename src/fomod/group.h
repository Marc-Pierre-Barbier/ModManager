#ifndef __GROUP_H__
#define __GROUP_H__

#include <libxml/parser.h>
#include "fomodTypes.h"
#include "xmlUtil.h"

typedef enum GroupType_t { ONE_ONLY, ANY, AT_LEAST_ONE, AT_MOST_ONE, ALL } GroupType_t;
typedef enum TypeDescriptor { OPTIONAL, MAYBE_USABLE, NOT_USABLE, REQUIRED, RECOMMENDED } TypeDescriptor_t;

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

int grp_parseGroup(xmlNodePtr groupNode, fomod_Group_t* group);
void grp_freeGroup(fomod_Group_t * group);


#endif
