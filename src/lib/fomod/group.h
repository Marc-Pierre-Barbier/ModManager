#ifndef __GROUP_H__
#define __GROUP_H__

#include <libxml/parser.h>
#include "xmlUtil.h"
#include <fomod.h>

int grp_parseGroup(xmlNodePtr groupNode, fomodGroup_t* group);
void grp_freeGroup(fomodGroup_t * group);

#endif
