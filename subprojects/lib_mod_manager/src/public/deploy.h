#ifndef __DEPLOY_H__
#define __DEPLOY_H__

#include "errorType.h"

typedef enum DeploymentErrors { OK, INVALID_APPID, GAME_NOT_FOUND, CANNOT_MOUNT, FUSE_NOT_INSTALLED, BUG, CANNOT_SYM_COPY } DeploymentErrors_t;

[[nodiscard]] DeploymentErrors_t deploy(int appid);
[[nodiscard]] error_t undeploy(int appid);
[[nodiscard]] error_t is_deployed(int appid, bool * status);
[[nodiscard]] char * get_deploy_target(int appid);

#endif
