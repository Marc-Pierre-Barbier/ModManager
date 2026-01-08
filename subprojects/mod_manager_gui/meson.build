project('mod_manager_gui', [ 'c', 'cpp' ],
	default_options : [
		'c_std=gnu2x',
		'cpp_std=c++2a,c++23'
	],
)

glib2_dep = dependency('glib-2.0')
gtk4_dep = dependency('gtk4')
libadwaita_dep = dependency('libadwaita-1')
gio_dep = dependency('gio-2.0')

libmodmanager_proj = subproject('lib_mod_manager')
libmodmanager_dep = libmodmanager_proj.get_variable('lib_mod_manager_dep')
path_utils_proj = subproject('path_utils')
lib_path_utils_dep = path_utils_proj.get_variable('lib_path_utils_dep')

gui_sources = files(
	'src/main.c',
	'src/game_tab.c',
	'src/fomod_install.cpp',
	'src/mod_tab.c',
	'src/plugin_tab.c',
	'src/settings_tab.c'
)

executable(
	'modManager-gui',
	sources: gui_sources,
	dependencies: [ glib2_dep, gtk4_dep, libadwaita_dep, gio_dep, libmodmanager_dep, lib_path_utils_dep ],
	install : true,
)
