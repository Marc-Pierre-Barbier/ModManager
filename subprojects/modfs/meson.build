project('modfs', ['cpp'],
	default_options : [
		'cpp_std=c++2a,c++23'
	],
)

cpp_std = get_option('cpp_std')

message('C++ standard: ' + cpp_std)

fuse3_dep = dependency('fuse3')

sources = files(
	'src/main.cpp',
)

executable(
	'modfs',
	sources: sources,
	dependencies: [ fuse3_dep ],
	install : true,
)
