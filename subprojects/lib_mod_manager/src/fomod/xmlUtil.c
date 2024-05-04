#include "xmlUtil.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static char * trim_start(const char * string) {
	const char * it = string;
	while(*it != '\0' && (*it == ' ' || *it == '\t' || *it == '\n'))
		it++;
	unsigned long length = strlen(it) + 1; // add space for \0
	char * trimmed = g_malloc(length);
	memcpy(trimmed, it, length);
	return trimmed;
}

char * xml_free_and_dup(xmlChar * line) {
	//some nodes start with cariage return and indentation leading to a massive amount of white spaces being read.
	char * free = trim_start((const char *) line);
	xmlFree(line);
	return free;
}

fomod_Order_t fomod_get_order(const char * order) {
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
void xml_fix_path(char * path) {
	while(*path != '\0') {
		if(*path == '\\')*path = '/';
		path++;
	}
}

int fomod_count_until_null(char ** pointers) {
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
bool xml_validate_node(xmlNodePtr * node, bool skip_text, const char * names, ...) {
	va_list names_ptr;

	//skipping text nodes
	while(*node != NULL && xmlStrcmp((*node)->name, (const xmlChar *)"text") == 0) {
		if(skip_text) {
			(*node) = (*node)->next;
		} else {
			//could not skip and the node was a text node.
			return false;
		}
	}

	if(*node == NULL) {
		return true;
	}

	va_start(names_ptr, names);

	const char * validName = names;
	while(validName != NULL) {
		if(xmlStrcmp((*node)->name, (const xmlChar *)validName) == 0) {
			va_end(names_ptr);
			return true;
		}
		validName = va_arg(names_ptr, char *);
	}

	va_end(names_ptr);
	return false;
}
