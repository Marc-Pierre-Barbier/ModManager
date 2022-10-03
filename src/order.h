#ifndef __ORDER_H__
#define __ORDER_H__

#include <glib.h>

#define ORDER_FILE "__ORDER__"

//This file manager the load order of mods.
// the current solution is using ORDER_FILE to store a number corresponding to the mod
// this might be sub-optimal but at least it will work just fine

/**
 * @brief update the mods ids and return the mods in order
 *
 * @return GList of char containing the name of the mod folder in order
 */
GList * listMods(int appid);
void swapPlace(int appid, int modId, int modId2);

#endif
