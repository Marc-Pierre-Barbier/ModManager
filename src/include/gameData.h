#ifndef __GAME_DATA_H__
#define __GAME_DATA_H__


#include "errorType.h"

/**
 * @brief Get the path to the data folder
 * @param appid appid of the game
 * @param destination pointer to a null char * variable. it's value will be allocated with malloc
 * @return error_t
 */
error_t gameData_getDataPath(int appid, char ** destination);

#endif
