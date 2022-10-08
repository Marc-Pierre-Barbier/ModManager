#ifndef __DATA_PATH_H__
#define __DATA_PATH_H__

#include "main.h"

/**
 * @brief Get the path to the data folder
 * @param appid appid of the game
 * @param destination pointer to a null char * variable. it's value will be allocated with malloc
 * @return error_t
 */
error_t getDataPath(int appid, char ** destination);


#endif
