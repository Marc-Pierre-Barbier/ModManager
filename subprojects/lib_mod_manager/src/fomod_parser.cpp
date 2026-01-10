#include "fomod.hpp"
#include "fomodTypes.hpp"
#include <case_adapted_path.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <memory>
#include <libxml/parser.h>
#include <glib.h>
#include <optional>
#include <cstdlib>

static bool str_equal(const xmlChar * a, const std::string &b) {
	return b == reinterpret_cast<const char *>(a);
}

static std::optional<std::string> xmlGetProp(const xmlNode *node, const std::string &name) {
	auto value = xmlGetProp(node, (const xmlChar *)name.c_str());
	if(value == nullptr)
		return std::nullopt;
	std::string ret = (const char *)value;
	xmlFree(value);
	return ret;
}

std::optional<std::string> xmlNodeGetContentString(const xmlNode *cur) {
	auto value = xmlNodeGetContent(cur);
	if(value == nullptr)
		return std::nullopt;
	std::string ret((const char *)value);
	xmlFree(value);
	return ret;
}

void xml_fix_path(std::string &path) {
	std::replace(path.begin(), path.end(), '\\', '/');
}


static bool xml_validate_node(xmlNodePtr * node, bool skip_text_and_comments, std::initializer_list<std::string> names) {

	//skipping text nodes
	while(*node != NULL &&  (
		str_equal((*node)->name, "text") ||
		str_equal((*node)->name, "comment")
	)) {
		if(skip_text_and_comments) {
			(*node) = (*node)->next;
		} else {
			//could not skip and the node was a text node.
			return false;
		}
	}

	if(*node == NULL) {
		return true;
	}

	for(auto validName : names) {
		if(str_equal((*node)->name, validName))
			return true;
	}
	return false;
}

FOModOrder fomod_get_order(const std::string &order) {
	if(order == "Ascending") {
		return FOModOrder::ASC;
	} else if(order == "Explicit") {
		return FOModOrder::ORD;
	} else if(order == "Descending") {
		return FOModOrder::DESC;
	}
	return FOModOrder::UNKNOWN;
}

static TypeDescriptor get_descriptor(std::string descriptor) {
	if(descriptor == "Optional") {
		return TypeDescriptor::OPTIONAL;
	} else if(descriptor == "Required") {
		return TypeDescriptor::REQUIRED;
	} else if(descriptor == "Recommended") {
		return TypeDescriptor::RECOMMENDED;
	} else if(descriptor == "CouldBeUsable") {
		return TypeDescriptor::MAYBE_USABLE;
	} else if(descriptor == "NotUsable") {
		return TypeDescriptor::NOT_USABLE;
	} else {
		return TypeDescriptor::UNKNOWN;
	}
}

static bool parse_condition_flags(FOModPlugin &plugin, xmlNodePtr nodeElement) {
	xmlNodePtr flag_node = nodeElement->children;
	while(flag_node != NULL) {
		if(!xml_validate_node(&flag_node, true, {"flag"})) {
			return false;
		}
		if(flag_node == NULL)continue;

		std::string name = *xmlGetProp(flag_node, "name");
		std::string value = *xmlNodeGetContentString(flag_node);
		plugin.flags.emplace_back(name, value);
		flag_node = flag_node->next;
	}
	return true;
}

static bool parseGroupFiles(FOModPlugin &plugin, xmlNodePtr node_element) {
	xmlNodePtr file_node = node_element->children;
	for(; file_node != NULL; file_node = file_node->next) {
		if(!xml_validate_node(&file_node, true, {"folder", "file"})) {
			g_warning( "Unexpected node in files\n");
			return false;
		}
		if(file_node == NULL)break;


		FOModFile &file = plugin.files.emplace_back();

		file.destination = *xmlGetProp(file_node, "destination");
		file.source = *xmlGetProp(file_node, "source");

		//TODO: test if it's a number
		auto priority = xmlGetProp(file_node, "priority");
		if(!priority)
			file.priority = 0;
		else
			file.priority = atoi(priority->c_str());

		file.isFolder = str_equal(file_node->name, "folder");
	}
	return true;
}

static bool parseNodeElement(FOModPlugin &plugin, xmlNodePtr node_element) {
	if(str_equal(node_element->name, "description")) {
		plugin.description = *xmlNodeGetContentString(node_element);
	} else if(str_equal(node_element->name, "image")) {
		plugin.image = *xmlGetProp(node_element, "path");
	} else if(str_equal(node_element->name, "conditionFlags")) {
		return parse_condition_flags(plugin, node_element);
	} else if(str_equal(node_element->name, "files")) {
		return parseGroupFiles(plugin, node_element);
	} else if(str_equal(node_element->name, "typeDescriptor")) {
		//WEIRD SHIT
		xmlNodePtr typeNode = node_element->children;
		if(!xml_validate_node(&typeNode, true, {"type", "dependencyType"})) {
			g_warning( "Unexpected node in typeDescriptor: %s at %d\n", typeNode->name, typeNode->line);
			return false;
		}

		if(str_equal(typeNode->name, "dependencyType")) {
			//TODO: add support for it
			g_warning("Error using unsupported functionnality: %s\n", typeNode->name);
			return false;
		} else { // <type>
			plugin.type = get_descriptor(*xmlGetProp(typeNode, "name"));
		}

	}
	return true;
}

[[nodiscard]] static bool parse_visible_node(xmlNodePtr node, FOModStep &step) {
	for(xmlNodePtr dependency_node = node->children; dependency_node != NULL; dependency_node = dependency_node->next) {
		if(!xml_validate_node(&dependency_node, true, {"dependencies", "flagDependency"})) {
			g_warning("error in fomod_parser.cpp:%d\n", __LINE__);
			return false;
		}
		if(dependency_node == NULL)break;

		if(str_equal(dependency_node->name, "flagDependency")) {
			std::string name = *xmlGetProp(dependency_node, "flag");
			std::string value = *xmlGetProp(dependency_node, "value");
			step.required_flags.emplace_back(name, value);
			continue;
		}

		if(*xmlGetProp(dependency_node, "operator") == "and") {
			g_warning("error in fomod_parser.cpp:%d\n", __LINE__);
			return false;
		}
	
		for (xmlNodePtr required_flagsNode = dependency_node->children;required_flagsNode != NULL; required_flagsNode = required_flagsNode->next) {
			if(!xml_validate_node(&required_flagsNode, true, {"flagDependency"})) {
				g_warning("error in fomod_parser.cpp:%d\n", __LINE__);
				return false;
			}
			if(required_flagsNode == NULL)break;
	
			std::string name = *xmlGetProp(required_flagsNode, "flag");
			std::string value = *xmlGetProp(required_flagsNode, "value");
			step.required_flags.emplace_back(name, value);
		}
	}

	return true;
}

static GroupType get_group_type(std::string type) {
	if(type == "SelectAtLeastOne") {
		return GroupType::AT_LEAST_ONE;
	} else if(type == "SelectAtMostOne") {
		return GroupType::AT_MOST_ONE;
	} else if(type == "SelectExactlyOne") {
		return GroupType::ONE_ONLY;
	} else if(type == "SelectAll") {
		return GroupType::ALL;
	} else if(type == "SelectAny") {
		return GroupType::ANY;
	} else {
		return GroupType::UNKNOWN;
	}
}

[[nodiscard]] static bool parse_optional_file_group(xmlNodePtr node, FOModStep &step) {
	auto option_order = xmlGetProp(node, "order");
	if(!option_order) {
		option_order = "Ascending";
	}
	step.option_order = fomod_get_order(*option_order);
	
	for(xmlNodePtr xml_group = node->children;xml_group != NULL; xml_group = xml_group->next) {
		if(!xml_validate_node(&xml_group, true, {"group"})) {
			//TODO: handle error
			g_warning("parsing error: fomod_parser.cpp:%d\n", __LINE__);
			return false;
		}
		if(xml_group == NULL)break;

		auto &group = step.groups.emplace_back();
		group.name = *xmlGetProp(xml_group, "name");
		group.type = get_group_type(*xmlGetProp(xml_group, "type"));

		//plugins (plural)
		xmlNodePtr plugins_node = xml_group->children;
		if(!xml_validate_node(&plugins_node, true, {"plugins"})) {
			g_warning("parsing error: fomod_parser.cpp:%d\n", __LINE__);
			return false;
		}

		auto order = xmlGetProp(plugins_node, "order");
		if(order) {
			group.order = fomod_get_order(*order);
		} else {
			group.order = FOModOrder::ASC;
		}

		//plugin (singular)
		xmlNodePtr current_plugin = plugins_node->children;
		for(; current_plugin != NULL; current_plugin = current_plugin->next) {
			if(!xml_validate_node(&current_plugin, true, {"plugin"})) {
				g_warning("group.c: %d\n", __LINE__);
				return false;
			}
			if(current_plugin == NULL)break;

			FOModPlugin &plugin = group.plugins.emplace_back();
			plugin.name = *xmlGetProp(current_plugin, "name");

		
			for(xmlNodePtr nodeElement = current_plugin->children; nodeElement != NULL; nodeElement = nodeElement->next) {
				if(!parseNodeElement(plugin, nodeElement) != EXIT_SUCCESS)
					return false;
			}
		}
	}

	return true;
}

static bool parse_install_steps(FOMod &fomod, xmlNodePtr install_steps_node) {
	for(xmlNodePtr step_node = install_steps_node->children;step_node != NULL; step_node = step_node->next) {
		//skipping the text node
		if(!xml_validate_node(&step_node, true, {"installStep"})) {
			g_warning("parsing error: fomod_parser.cpp:%d\n", __LINE__);
			return false;
		}
		if(step_node == NULL)break;

		auto &step = fomod.steps.emplace_back();
		step.name = *xmlGetProp(step_node, "name");

		for(xmlNodePtr step_childrens = step_node->children; step_childrens != NULL; step_childrens = step_childrens->next) {
			if(str_equal(step_childrens->name, "visible")) {
				if(!parse_visible_node(step_childrens, step)) {
					return false;
				}
			} else if(str_equal(step_childrens->name, "optionalFileGroups")) {
				if(!parse_optional_file_group(step_childrens, step)) {
					return false;
				}
			}
		}
	}
	return true;
}

static bool parseDependencies(xmlNodePtr node, FOModCondFile &cond_file) {
	for(xmlNodePtr flag_node = node->children; flag_node != NULL; flag_node = flag_node->next) {
		if(!xml_validate_node(&flag_node, true, {"flagDependency"})) {
			printf("parsing error: fomod_parser.cpp:%d\n", __LINE__);
			return false;
		}
		if(flag_node == NULL)
			break;

		std::string name = *xmlGetProp(flag_node, "flag");
		std::string value = *xmlGetProp(flag_node, "value");
		cond_file.required_flags.emplace_back(name, value);
	}
	return true;
}

static bool parseFiles(xmlNodePtr node, FOModCondFile &cond_file) {
	for(xmlNodePtr filesNode = node->children; filesNode != NULL; filesNode = filesNode->next) {
		if(!xml_validate_node(&filesNode, true, {"folder", "file"})) {
			printf("parsing error: fomod_parser.cpp:%d\n", __LINE__);
			return false;
		}
		if(filesNode == NULL)
			break;

		FOModFile &file = cond_file.files.emplace_back();
		file.source = *xmlGetProp(filesNode, "source");
		file.destination = *xmlGetProp(filesNode, "destination");
		file.priority = 0;
		file.isFolder = xmlStrcmp(filesNode->name, (xmlChar *) "folder") == 0;
	}

	return true;
}

static bool parseConditionalInstalls(xmlNodePtr node, FOMod &fomod) {
	xmlNodePtr patterns = node->children;
	if(!xml_validate_node(&patterns, true, {"patterns"})) {
		printf("parsing error: fomod_parser.cpp:%d\n", __LINE__);
		return false;
	}
	if(patterns == NULL) {
		return true;
	}

	xmlNodePtr current_pattern = patterns->children;
	if(!xml_validate_node(&current_pattern, true, {"pattern"})) {
		printf("parsing error: fomod_parser.cpp:%d for node %s\n", __LINE__, current_pattern->name);
		return false;
	}
	
	for(;current_pattern != NULL; current_pattern = current_pattern->next) {
		if(!xml_validate_node(&current_pattern, true, {"pattern"})) {
			printf("parsing error: fomod_parser.cpp:%d for node %s\n", __LINE__, current_pattern->name);
			return false;
		}
		if(current_pattern == NULL) break;

		xmlNodePtr pattern_child = current_pattern->children;
		if(!xml_validate_node(&pattern_child, true, {"dependencies", "files"})) {
			printf("parsing error: fomod_parser.cpp:%d for node %s\n", __LINE__, pattern_child->name);
			return false;
		}

		if(pattern_child == NULL)
			return true;

		FOModCondFile &cond_file = fomod.cond_files.emplace_back();
		for(;pattern_child != NULL; pattern_child = pattern_child->next) {
			if(!xml_validate_node(&pattern_child, true, {"dependencies", "files"})) {
				printf("parsing error: fomod_parser.cpp:%d for node %s\n", __LINE__, pattern_child->name);
				return false;
			}
			if(pattern_child == NULL)
				break;

			if(str_equal(pattern_child->name, "dependencies")) {
				if(!parseDependencies(pattern_child, cond_file)) {
					return false;
				}
			} else if(str_equal(pattern_child->name, "files")) {
				if(!parseFiles(pattern_child, cond_file)) {
					return false;
				}
			}
			
		}
	}
	return true;
}

static void fomod_fix_case(FOMod &fomod, std::filesystem::path mod_dir) {
	auto fix_path = [mod_dir](std::filesystem::path path) {
		auto adapted = case_adapted_path(path.c_str(), mod_dir);
		if(adapted) {
			return *adapted;
		}
		return path;
	};

	for(auto &file : fomod.required_install_files) {
		xml_fix_path(file.source);
		xml_fix_path(file.destination);
		file.source = fix_path(file.source);
	}

	for(auto &step : fomod.steps) {
		for(auto &group : step.groups) {
			for(auto &plugin : group.plugins) {
				xml_fix_path(plugin.image);
				plugin.image = fix_path(plugin.image);
				for(auto &file : plugin.files) {
					xml_fix_path(file.source);
					xml_fix_path(file.destination);
					file.source = fix_path(file.source);
				}
			}
		}
	}
}

static void fomod_sortSteps(FOMod &fomod) {
	switch(fomod.step_order) {
	case FOModOrder::ASC:
		std::sort(fomod.steps.begin(), fomod.steps.end(), [](const FOModStep &a, const FOModStep &b)
		{
			return strcmp(a.name.c_str(), b.name.c_str()) < 0;
		});
		break;
	case FOModOrder::DESC:
		std::sort(fomod.steps.begin(), fomod.steps.end(), [](const FOModStep &a, const FOModStep &b)
		{
			return strcmp(a.name.c_str(), b.name.c_str()) > 0;
		});
		break;
	case FOModOrder::ORD:
		//ord mean that we keep the curent order, so no need to sort anything.
		break;
	default:
		g_warning("Unknown sort order, not sorting");
	}
}

static void fomod_sortGroup(FOModGroup &group) {
	switch(group.order) {
	case FOModOrder::ASC:
		std::sort(group.plugins.begin(), group.plugins.end(), [](const FOModPlugin &a, const FOModPlugin &b)
		{
			return strcmp(a.name.c_str(), b.name.c_str()) < 0;
		});
		break;
	case FOModOrder::DESC:
		std::sort(group.plugins.begin(), group.plugins.end(), [](const FOModPlugin &a, const FOModPlugin &b)
		{
			return strcmp(a.name.c_str(), b.name.c_str()) > 0;
		});
		break;
	case FOModOrder::ORD:
		//ord mean that we keep the curent order, so no need to sort anything.
		break;
	default:
		g_warning("Unknown sort order, not sorting");
	}
}

std::optional<std::shared_ptr<FOMod>> fomod_parse(std::filesystem::path fomod_file) {
	auto fomod = std::make_shared<FOMod>();

	xmlDocPtr doc = xmlParseFile(fomod_file.c_str());
	if(doc == NULL) {
		g_warning( "Document is not a valid xml file\n");
		return std::nullopt;
	}

	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		g_warning( "emptyDocument\n");
		xmlFreeDoc(doc);
		return std::nullopt;
	}

	if(!str_equal(cur->name, "config")) {
		g_warning( "document of the wrong type, root node is '%s' instead of config\n", cur->name);
		xmlFreeDoc(doc);
		return std::nullopt;
	}

	for(cur = cur->children; cur != NULL; cur = cur->next) {
		if(str_equal(cur->name, "moduleName")) {
			auto file = *xmlNodeGetContentString(cur);
			fomod->module_name = file;
		} else if(str_equal(cur->name, "moduleImage")) {
			fomod->module_image = *xmlGetProp(cur, "path");
		} else if(str_equal(cur->name, "requiredInstallFiles")) {
			xmlNodePtr required_install_file = cur->children;
			for(; required_install_file != NULL; required_install_file = required_install_file->next) {
				if(!xml_validate_node(&required_install_file, true, {"folder", "file"})) {
					g_warning("parsing error: fomod_parser.cpp:%d when parsing line: %d node: %s\n", __LINE__, required_install_file->line, required_install_file->name);
					return std::nullopt;
				}
				if(required_install_file == NULL) break;

				auto &file = fomod->required_install_files.emplace_back();
				file.source = *xmlGetProp(required_install_file, "source");
				file.destination = *xmlGetProp(required_install_file, "destination");
				file.isFolder = str_equal(required_install_file->name, "folder");
				//minimum priority so that it get installed first and everything can override it
				file.priority = std::numeric_limits<int>::min();
			}
		} else if(str_equal(cur->name, "installSteps")) {
			if(!fomod->steps.empty()) {
				g_warning( "Multiple 'installSteps' tags\n");
				//TODO: handle error
				return std::nullopt;
			}

			const std::string step_order = *xmlGetProp(cur, "order");
			fomod->step_order = fomod_get_order(step_order);

			if(!parse_install_steps(*fomod, cur)) {
				g_warning( "Failed to parse the install steps\n");
				return std::nullopt;
			}

		} else if(str_equal(cur->name, "conditionalFileInstalls")) {
			if(!parseConditionalInstalls(cur, *fomod))
				return std::nullopt;
		}
	}

	//steps are in a specific order in fomod config. we need to make sure we respected this order
	fomod_sortSteps(*fomod);
	for(auto step : fomod->steps)
		for(auto group : step.groups)
			fomod_sortGroup(group);

	fomod_fix_case(*fomod, fomod_file.parent_path().parent_path());

	//TODO: dealloc properly
	xmlFreeDoc(doc);
	return fomod;
}