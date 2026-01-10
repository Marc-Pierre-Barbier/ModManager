#pragma once

#include <filesystem>
#include <vector>
enum class OverlayErrors { SUCCESS, NOT_INSTALLED, FAILURE };

/**
 * @brief Overlayfs is what make it possible to deploy to the game files without altering anything.
 * it allows us to overlay multiple folder over the game files. like appling filters to an image.
 * @param sources a null ended table of folder that will overlay of the game files
 * @param dest the overlayed folder(the game data folder)
 * @param upperdir the director that will store the changed files
 * @return int error code
 */
OverlayErrors overlay_mount(const std::vector<std::filesystem::path> &sources, const std::filesystem::path &dest, const std::filesystem::path &upperdir);
