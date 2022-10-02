#ifndef __FOMOD_TYPES_H__
#define __FOMOD_TYPES_H__

#include "stdbool.h"
#include "xmlUtil.h"

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



typedef struct FOModCondFile {
	FOModFlag_t * requiredFlags;
	unsigned int flagCount;
	FOModFile_t * files;
	unsigned int fileCount;
} FOModCondFile_t;

#endif
