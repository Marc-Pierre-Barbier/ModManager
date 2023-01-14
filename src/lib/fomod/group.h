#ifndef __GROUP_H__
#define __GROUP_H__

#include <libxml/parser.h>
#include "xmlUtil.h"
#include <fomod.h>

int grp_parseGroup(xmlNodePtr groupNode, fomod_Group_t* group);
void grp_freeGroup(fomod_Group_t * group);

#endif
