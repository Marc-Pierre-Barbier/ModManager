#ifndef __LOAD_ORDER_H__
#define __LOAD_ORDER_H__

#include <glib.h>
#include <errorType.h>
#include <stdbool.h>

typedef struct order_mod_entry {
    bool activated;
    char * filename;

} order_plugin_entry_t;

/**
 * @brief find all plugins in the data folder of the game
 * @param appid the appid of the game
 * @param order a pointer to a null Glist *. in which there will be the list of esm files.
 */
error_t order_listPlugins(int appid, GList ** list)      __attribute__((warn_unused_result));

/**
 * @brief fetch the load order of the game it might be null if setLoadOrder was never called.
 * @param appid the appid of the game
 * @param order a pointer to a null Glist *. in which there will be the list of order_mod_entry_t.
 */
error_t order_getLoadOrder(int appid, GList ** order)    __attribute__((warn_unused_result));

/**
 * @brief change the plugin load order of the game
 * @param appid the appid of the game
 * @param loadOrder the load order a Glist of order_plugin_entry_t
 */
error_t order_setLoadOrder(int appid, GList * loadOrder) __attribute__((warn_unused_result));

/**
 * @brief List all dependencies for a esm mod.
 * @param esmPath path to the esm file
 * @param dependencies a pointer to a null Glist *. in which there will be the list of esm files.
 * free it using g_list_free_full(dependencies, free);
 */
error_t order_getModDependencies(const char * esmPath, GList ** dependencies) __attribute__((warn_unused_result));

/**
 * @brief free a order_mod_entry_t* allocated with g_malloc to use with g_list_free_full(GList *list, (GDestroyNotify)order_free_mod_entry)
 */
void order_free_plugin_entry(order_plugin_entry_t *);

#endif
