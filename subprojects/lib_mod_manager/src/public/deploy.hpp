#pragma once

#include <filesystem>

#include "errorType.h"

enum class DeploymentErrors: int { OK, INVALID_APPID, GAME_NOT_FOUND, CANNOT_MOUNT, MODFS_NOT_INSTALLED, BUG, CANNOT_SYM_COPY };


[[nodiscard]] DeploymentErrors deploy(int appid);
[[nodiscard]] error_t undeploy(int appid);
[[nodiscard]] std::optional<bool> is_deployed(int appid);
[[nodiscard]] std::filesystem::path get_deploy_target(int appid);

//temporary C interface that will be removed when meson have a good crate intergration so that i can use the cxx crate
extern "C" {
    #include <deploy.h>
}
