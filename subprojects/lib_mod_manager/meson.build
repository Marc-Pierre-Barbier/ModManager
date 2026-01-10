project('libmodManager', [ 'c', 'cpp' ],
	default_options : [
		'c_std=gnu2x',
		'cpp_std=c++23'
	],
)

glib2_dep = dependency('glib-2.0')
libxml_dep = dependency('libxml-2.0')
gio_dep = dependency('gio-2.0')
libarchive_dep = dependency('libarchive')
path_utils_proj = subproject('path_utils')
lib_path_utils_dep = path_utils_proj.get_variable('lib_path_utils_dep')

lib_sources = files(
	'src/steam.c',
	'src/loadOrder.c',
	'src/overlayfs.cpp',
	'src/file.c',
	'src/mods.c',
	'src/archives.c',
	'src/fomod.cpp',
	'src/deploy.cpp',
	'src/game_executables.c',
	'src/fomod_parser.cpp'
)


# Headers files to export in the include folder
lib_headers = files(
	'src/public/modmanager.h',
	'src/public/constants.h',
	'src/public/deploy.hpp',
	'src/public/errorType.h',
	'src/public/file.h',
	'src/public/fomod.hpp',
	'src/public/fomodTypes.hpp',
	'src/public/loadOrder.h',
	'src/public/mods.h',
	'src/public/steam.h',
	'src/public/game_executables.h',
)

incdir = [ include_directories('src/public') ]

lib_mod_manager = library(
	'modManager',
	sources: [lib_sources + lib_headers],
	dependencies: [ glib2_dep, libxml_dep, gio_dep, libarchive_dep, lib_path_utils_dep ],
	include_directories: incdir,
	install : true
)

install_headers(lib_headers, subdir: 'modmanager')

lib_mod_manager_dep = declare_dependency(include_directories : incdir, link_with : lib_mod_manager)
