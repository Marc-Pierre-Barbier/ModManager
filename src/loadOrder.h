#ifndef __LOAD_ORDER_H__
#define __LOAD_ORDER_H__

#include <glib.h>
#include "main.h"

error_t listPlugins(int appid, GList ** list)      __attribute__((warn_unused_result));
error_t getLoadOrder(int appid, GList ** order)    __attribute__((warn_unused_result));
error_t setLoadOrder(int appid, GList * loadOrder) __attribute__((warn_unused_result));

#endif
