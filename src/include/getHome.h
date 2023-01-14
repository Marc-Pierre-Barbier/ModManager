#ifndef __GET_HOME_H__
#define __GET_HOME_H__

/**
 * @brief Get the user home dir regardless if he used sudo.
 *
 * @return char* path to the home dir
 */
char * getHome(void);

#endif
