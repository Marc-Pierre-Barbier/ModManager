project('libmodManager', 'c',
	default_options : [
		'c_std=gnu2x' # 23 but it's not available yet
	],
)

glib2_dep = dependency('glib-2.0')
libxml_dep = dependency('libxml-2.0')
gio_dep = dependency('gio-2.0')
libarchive_dep = dependency('libarchive')


lib_sources = files(
	'src/steam.c',
	'src/loadOrder.c',
	'src/overlayfs.c',
	'src/file.c',
	'src/mods.c',
	'src/archives.c',
	'src/fomod.c',
	'src/deploy.c',
	'src/fomod/group.c',
	'src/fomod/xmlUtil.c',
	'src/fomod/parser.c'
)


# Headers files to export in the include folder
lib_headers = files(
	'src/public/modmanager.h',
	'src/public/constants.h',
	'src/public/deploy.h',
	'src/public/errorType.h',
	'src/public/file.h',
	'src/public/fomod.h',
	'src/public/fomodTypes.h',
	'src/public/loadOrder.h',
	'src/public/mods.h',
	'src/public/steam.h'
)

incdir = [ include_directories('src/public') ]

lib_mod_manager = library(
	'modManager',
	sources: [lib_sources + lib_headers],
	dependencies: [ glib2_dep, libxml_dep, gio_dep, libarchive_dep ],
	include_directories: incdir,
	install : true
)

install_headers(lib_headers, subdir: 'modmanager')

lib_mod_manager_dep = declare_dependency(include_directories : incdir, link_with : lib_mod_manager)
