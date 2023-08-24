#ifndef __GET_HOME_H__
#define __GET_HOME_H__

#include <gio/gio.h>

/**
 * @brief Get the user home dir regardless if he used sudo.
 *
 * @return char* path to the home dir
 */
GFile * audit_get_home(void);

#endif
