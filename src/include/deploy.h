#ifndef __DEPLOY_H__
#define __DEPLOY_H__

#include <glib.h>

typedef enum deploymentErrors { INVALID_APPID, GAME_NOT_FOUND, OK, CANNOT_MOUNT, FUSE_NOT_INSTALLED, BUG } deploymentErrors_t;

[[nodiscard]] deploymentErrors_t deploy(char * appIdStr, GList ** ignoredMods);

#endif
