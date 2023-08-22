#ifndef __ORDER_H__
#define __ORDER_H__

#include <glib.h>
#include <stdbool.h>
#include <errorType.h>

#define ORDER_FILE "__ORDER__"

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
GList * mods_list(int appid) __attribute__((warn_unused_result));

/**
 * @brief Change the mod order by swaping two mod
 *
 * @param appid the game
 * index of the mod in the mod list.
 * @param modId
 * @param modId2
 */
error_t mods_swap_place(int appid, int modId, int modId2)  __attribute__((warn_unused_result));

error_t mods_enable_mod(int appid, int modId)  __attribute__((warn_unused_result));
error_t mods_disable_mod(int appid, int modId)  __attribute__((warn_unused_result));
error_t mods_remove_mod(int appid, int modId)  __attribute__((warn_unused_result));
char * mods_get_mods_folder(int appid)  __attribute__((warn_unused_result));
char * mods_get_mod_folder(int appid, int mod_id)  __attribute__((warn_unused_result));
#endif
