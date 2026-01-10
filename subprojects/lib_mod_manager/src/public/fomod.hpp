#pragma once

#include "fomodTypes.hpp"

#include <memory>
#include <filesystem>
#include <optional>

/**
 * @brief parse a fomod xml file (found inside the mod folder /fomod/moduleconfig.xml (everything should be lowercase))
 */
std::optional<std::shared_ptr<FOMod>> fomod_parse(std::filesystem::path fomod_file);

void fomod_process_cond_files(const FOMod &fomod, const std::vector<FOModFlag> &flag_list, std::vector<FOModFile> &pending_file_operations);

/**
 * @brief FOModFile_t have a priority option and this function execute the file operation while taking this into account.
 * this create a new mod with the __FOMOD postfix in the name
 * @param pendingFileOperations list of FOModFile_t to process
 * @param modFolder mod_name
 * @return error code
 */
void fomod_execute_file_operations(std::vector<FOModFile> &pending_file_operations, const int mod_id, const int appid);
