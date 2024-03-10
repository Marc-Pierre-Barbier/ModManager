#ifndef __DEPLOY_H__
#define __DEPLOY_H__

#include "errorType.h"
#include <glib.h>

typedef enum DeploymentErrors { INVALID_APPID, GAME_NOT_FOUND, OK, CANNOT_MOUNT, FUSE_NOT_INSTALLED, CANNOT_UNMOUNT, BUG, CANNOT_SYM_COPY } DeploymentErrors_t;

[[nodiscard]] DeploymentErrors_t deploy(int appid);
[[nodiscard]] error_t undeploy(int appid);
[[nodiscard]] error_t is_deployed(int appid, bool * status);

#endif
