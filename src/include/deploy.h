#ifndef __DEPLOY_H__
#define __DEPLOY_H__

#include "errorType.h"
#include <glib.h>

typedef enum DeploymentErrors { INVALID_APPID, GAME_NOT_FOUND, OK, CANNOT_MOUNT, FUSE_NOT_INSTALLED, BUG } DeploymentErrors_t;

[[nodiscard]] DeploymentErrors_t deploy(int appid);
[[nodiscard]] error_t undeploy(int appid);

#endif
