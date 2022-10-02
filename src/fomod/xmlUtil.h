#ifndef __XML_UTIL_H__
#define __XML_UTIL_H__

#include <libxml/parser.h>
#include <stdbool.h>


typedef enum FOModOrder { ASC, DESC, ORD } FOModOrder_t;

bool validateNode(xmlNodePtr * node, bool skipText, const char * names, ...);
char * freeAndDup(xmlChar * line);
FOModOrder_t getFOModOrder(const char * order);
void fixPath(char * path);
int countUntilNull(void * pointers);

#endif
