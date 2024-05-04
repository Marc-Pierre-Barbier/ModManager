#ifndef __GROUP_H__
#define __GROUP_H__

#include <libxml/parser.h>
#include "xmlUtil.h"
#include <fomod.h>

int grp_parse_group(xmlNodePtr groupNode, FomodGroup_t* group);
void grp_free_group(FomodGroup_t * group);

#endif
