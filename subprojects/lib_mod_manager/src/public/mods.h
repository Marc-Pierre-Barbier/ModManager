#ifndef __ORDER_H__
#define __ORDER_H__

#include <glib.h>
#include <gio/gio.h>

#include "errorType.h"

//This file manager the load order of mods.
// the current solution is using ORDER_FILE to store a number corresponding to the mod
// this might be sub-optimal but at least it will work just fine

typedef struct mods_mod_detail {
    bool is_present; //set to false if mod_name or appid is invalid
    bool is_fomod;
    bool has_fomodfile;
    bool is_activated;
    bool has_fomod_sibling;
} mods_mod_detail_t;

mods_mod_detail_t mods_mod_details(const int appid, int modId) ;

/**
 * @brief update the mods ids and return the mods in order
 *
 * @return GList of char containing the name of the mod folder in order
 */
[[nodiscard]] GList * mods_list(int appid);

/**
 * @brief Change the mod order by swaping two mod
 *
 * @param appid the game
 * index of the mod in the mod list.
 * @param modId
 * @param modId2
 */
[[nodiscard]] error_t mods_swap_place(int appid, int modId, int modId2);

/**
 * @brief Add a mod to the folder defined in main.h
 *
 * @param filePath path to the mod file
 * @param appId game for which the mod is destined to be used with.
 */
[[nodiscard]] error_t mods_add_mod(GFile * filePath, int appId);

[[nodiscard]] error_t mods_enable_mod(int appid, int modId);
[[nodiscard]] error_t mods_disable_mod(int appid, int modId);
[[nodiscard]] error_t mods_remove_mod(int appid, int modId);
[[nodiscard]] GFile * mods_get_mods_folder(int appid);
[[nodiscard]] GFile * mods_get_mod_folder(int appid, int mod_id);
#endif
