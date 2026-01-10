#include "case_adapted_path.hpp"
#include "case_adapted_path.h"

#include <string.h>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

std::optional<fs::path> case_adapted_path(fs::path fspath, fs::path refrence_dir) {
	std::vector<std::string> path;

	fs::path badpath = fspath;
	while(!badpath.empty() && badpath != "/" && badpath != ".") { //fail in precense of tailing sperators
		auto name = badpath.filename();
		if(!name.empty()) //handle tailing sperators
			path.push_back(name);
		badpath = badpath.parent_path();
	}

	while(!path.empty()) {
		std::string filename = *path.rbegin();
		transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
		path.pop_back();

		bool found = false;
		for(auto entry : fs::directory_iterator(refrence_dir)) {
			const auto on_disk_name = entry.path().filename().string();
			std::string on_disk_name_lower = on_disk_name;
			transform(on_disk_name_lower.begin(), on_disk_name_lower.end(), on_disk_name_lower.begin(), ::tolower);

			if(filename == on_disk_name_lower) {
				refrence_dir /= on_disk_name;
				found = true;
				break;
			}
		}

		if(!found) {
			return std::nullopt;
		}
	}

	return refrence_dir;
}


extern "C" {
	char * case_adapted_path(const char * fspath, const char * refrence_dir) {
		auto path = case_adapted_path(fs::path(fspath), fs::path(refrence_dir));
		if(path) {
			return strdup(path->c_str());
		} else {
			return nullptr;
		}
	}
}

