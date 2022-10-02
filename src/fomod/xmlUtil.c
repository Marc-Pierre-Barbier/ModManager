#include "xmlUtil.h"

#include <string.h>

char * freeAndDup(xmlChar * line) {
	char * free = strdup((const char *) line);
	xmlFree(line);
	return free;
}

FOModOrder_t getFOModOrder(const char * order) {
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
void fixPath(char * path) {
	while(*path != '\0') {
		if(*path == '\\')*path = '/';
		path++;
	}
}

int countUntilNull(void * pointers) {
	int i = 0;
	while(pointers != NULL) {
		pointers++;
		i++;
	}
	return i;
}

//names cannot contain false
//need to be null terminated
bool validateNode(xmlNodePtr * node, bool skipText, const char * names, ...) {
	va_list namesPtr;

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
