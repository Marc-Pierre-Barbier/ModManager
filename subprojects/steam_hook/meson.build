project('steam_hook', 'rust',
	default_options : [
		'c_std=gnu2x'
	],
)

libmodmanager_proj = subproject('lib_mod_manager')
lib_mod_manager_dep = libmodmanager_proj.get_variable('lib_mod_manager_dep')

hook_sources = files(
	'src/main.rs'
)

executable(
	'modmanager_steamhook',
	sources: hook_sources,
	dependencies: [ lib_mod_manager_dep ],
	install : true
)
