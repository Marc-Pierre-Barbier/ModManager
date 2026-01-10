#include <filesystem>
#include <sys/mount.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <glib.h>

#include "overlayfs.hpp"
namespace fs = std::filesystem;

OverlayErrors overlay_mount(const std::vector<fs::path> &sources, const fs::path &dest, const fs::path &upperdir) {
	std::string lower_dirs;

	for(auto &source : sources) {
		lower_dirs += ":" + source.string();
	}

	if(!lower_dirs.empty()) { //remove the first :
		lower_dirs.erase(0, 1);
	}

	std::string mount_data = "--lowerdirs=\"" + lower_dirs + "\" --upperdir=\"" + upperdir.string() + "\"";
	std::string full_command = "modfs " + mount_data + " " + dest.string();

	OverlayErrors result = OverlayErrors::SUCCESS;

	if(system("which modfs") == 0) {
		int return_value = system(full_command.c_str());
		if(return_value != 0) {
			result = OverlayErrors::FAILURE;
		}
	} else {
		result = OverlayErrors::NOT_INSTALLED;
	}
	return result;
}
