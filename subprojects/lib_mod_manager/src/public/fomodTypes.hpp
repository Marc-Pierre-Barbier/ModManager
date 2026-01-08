#pragma once

#include <string>
#include <vector>

enum class GroupType { ONE_ONLY, ANY, AT_LEAST_ONE, AT_MOST_ONE, ALL, UNKNOWN };
enum class TypeDescriptor { OPTIONAL, MAYBE_USABLE, NOT_USABLE, REQUIRED, RECOMMENDED, UNKNOWN };
enum class FOModOrder { ASC, DESC, ORD, UNKNOWN };

struct FOModFlag {
	std::string name;
	std::string value;
};

struct FOModFile {
	std::string source;
	std::string destination;
	int priority = 0;
	bool isFolder;
};

struct FOModCondFile {
	std::vector<FOModFlag> required_flags;
	std::vector<FOModFile> files;
};

struct FOModPlugin {
	std::string description;
	std::string image;
	std::vector<FOModFlag> flags;
	std::vector<FOModFile> files;
	TypeDescriptor type;
	std::string name;
};

//combine group and "plugins"
struct FOModGroup {
	std::vector<FOModPlugin> plugins;
	GroupType type;
	std::string name;
	FOModOrder order;
};

//combine installStep and optionalFileGroups
struct FOModStep {
	FOModOrder option_order;
	std::vector<FOModGroup> groups;
	std::string name;
	std::vector<FOModFlag> required_flags;
};

struct FOMod {
	std::string module_name;
	std::string module_image;
	std::vector<FOModFile> required_install_files;
	FOModOrder step_order;
	std::vector<FOModStep> steps;
	std::vector<FOModCondFile> cond_files;
};
