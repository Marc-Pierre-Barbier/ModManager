typedef enum cDeploymentErrors { OK, INVALID_APPID, GAME_NOT_FOUND, CANNOT_MOUNT, MODFS_NOT_INSTALLED, BUG, CANNOT_SYM_COPY } cDeploymentErrors_t;

cDeploymentErrors_t _deploy(int appid);
error_t _undeploy(int appid);
error_t _is_deployed(int appid, bool * status);
char * _get_deploy_target(int appid);