#ifndef __XML_UTIL_H__
#define __XML_UTIL_H__

#include <libxml/parser.h>
#include <stdbool.h>


typedef enum FOModOrder { ASC, DESC, ORD } fomod_Order_t;

/**
 * @brief
 *
 * @param node a pointer to the current node pointer (xmlNode **)
 * @param skipText if a text element is found skip it.
 * @param names variadic of the valid names.
 * @return return true if it found a valid node
 */
bool xml_validateNode(xmlNodePtr * node, bool skipText, const char * names, ...);

/**
 * @brief Free memory of and xmlChar and return a strdup version. just to make sure there is nothing remaining in libxml
 */
char * xml_freeAndDup(xmlChar * line);


fomod_Order_t fomod_getOrder(const char * order);

/**
 * @brief replace / by \
 * @param path
 */
void xml_fixPath(char * path);

/**
 * @brief Count the number of step before null
 *
 * @param pointers pointer to the list
 * @param typeSize size of each element of the list
 * @return size
 */
int fomod_countUntilNull(void * pointers, size_t typeSize);

#endif
