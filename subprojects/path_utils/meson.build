project('libpathutils', [ 'cpp' ],
	default_options : [
		'cpp_std=c++2a,c++23'
	],
)

lib_sources = files(
	'src/case_adapted_path.cpp'
)


# Headers files to export in the include folder
lib_headers = files(
	'src/public/case_adapted_path.h',
	'src/public/case_adapted_path.hpp'
)

incdir = [ include_directories('src/public') ]

lib_path_utils = static_library(
	'modManager',
	sources: [lib_sources + lib_headers],
	include_directories: incdir,
	install : true,
)

install_headers(lib_headers, subdir: 'modmanager')

lib_path_utils_dep = declare_dependency(include_directories : incdir, link_with : lib_path_utils)
