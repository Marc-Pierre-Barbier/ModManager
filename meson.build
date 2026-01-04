project('ModManager', version : '0.1',
	meson_version: '>=1.3.0')

if get_option('buildtype').startswith('debug')
	add_global_arguments('-DDEBUG', language : 'c')
endif

subproject('lib_mod_manager')
subproject('mod_manager_gui')
subproject('steam_hook')

#disabled due to crunchy not being buildable with meson yet
#dependency('libloot-0.28-rs')
# 0.14 and latter use rust too.
# anything before that is too old

