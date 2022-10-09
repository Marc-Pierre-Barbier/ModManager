#ifndef __FOMOD_TYPES_H__
#define __FOMOD_TYPES_H__

#include "stdbool.h"
#include "xmlUtil.h"

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

#endif
