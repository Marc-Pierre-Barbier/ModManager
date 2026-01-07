project('ModManager', version : '0.1',
	meson_version: '>=1.3.0')

if get_option('buildtype').startswith('debug')
	add_global_arguments('-DDEBUG', language : 'c')
endif

if get_option('use_sanitizers')
	add_global_arguments('-fno-omit-frame-pointer', language: 'c')
	add_global_arguments('-fno-optimize-sibling-calls', language: 'c')
	add_global_arguments('-fsanitize=address', language: 'c')
	add_global_link_arguments('-fsanitize=address', language: 'c')
endif

subproject('lib_mod_manager')
subproject('mod_manager_gui')
subproject('steam_hook')
subproject('modfs')
