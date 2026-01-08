project('modfs', ['cpp'],
	default_options : [
		'cpp_std=c++2a,c++23'
	],
)

cpp_std = get_option('cpp_std')

message('C++ standard: ' + cpp_std)

fuse3_dep = dependency('fuse3')
path_utils_proj = subproject('path_utils')
lib_path_utils_dep = path_utils_proj.get_variable('lib_path_utils_dep')

sources = files(
	'src/main.cpp',
)

executable(
	'modfs',
	sources: sources,
	dependencies: [ fuse3_dep, lib_path_utils_dep ],
	install : true,
)
