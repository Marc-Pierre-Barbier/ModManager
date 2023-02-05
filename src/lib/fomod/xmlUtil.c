#include "xmlUtil.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

char * xml_freeAndDup(xmlChar * line) {
	char * free = strdup((const char *) line);
	xmlFree(line);
	return free;
}

fomod_Order_t fomod_getOrder(const char * order) {
	if(order == NULL || strcmp(order, "Ascending") == 0) {
		return ASC;
	} else if(strcmp(order, "Explicit") == 0) {
		return ORD;
	} else if(strcmp(order, "Descending") == 0) {
		return DESC;
	}
	return -1;
}

//replace \ in the path by /
void xml_fixPath(char * path) {
	while(*path != '\0') {
		if(*path == '\\')*path = '/';
		path++;
	}
}

int fomod_countUntilNull(char ** pointers) {
	if(pointers == NULL) return 0;
	int i = 0;
	char ** arithmetic = pointers;
	while(*arithmetic != NULL) {
		arithmetic++;
		i++;
	}
	return i;
}

//names cannot contain false
//need to be null terminated
bool xml_validateNode(xmlNodePtr * node, bool skipText, const char * names, ...) {
	va_list namesPtr;

	//skipping text nodes
	while(*node != NULL && xmlStrcmp((*node)->name, (const xmlChar *)"text") == 0) {
		if(skipText) {
			(*node) = (*node)->next;
		} else {
			//could not skip and the node was a text node.
			return false;
		}
	}

	if(*node == NULL) {
		return true;
	}

	va_start(namesPtr, names);

	const char * validName = names;
	while(validName != NULL) {
		if(xmlStrcmp((*node)->name, (const xmlChar *)validName) == 0) {
			va_end(namesPtr);
			return true;
		}
		validName = va_arg(namesPtr, char *);
	}

	va_end(namesPtr);
	return false;
}
