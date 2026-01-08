#include <filesystem>
#include <optional>

std::optional<std::filesystem::path> case_adapted_path(std::filesystem::path fspath, std::filesystem::path refrence_dir);