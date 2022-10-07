#ifndef __INSTALL_H__
#define __INSTALL_H__
#include <stdlib.h>
#include "main.h"
#define INSTALLED_FLAG_FILE "__DO_NOT_REMOVE__"

/**
 * @brief Add a mod to the folder defined in main.h
 *
 * @param filePath path to the mod file
 * @param appId game for which the mod is destined to be used with.
 */
error_t addMod(char * filePath, int appId);

#endif
