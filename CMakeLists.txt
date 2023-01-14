cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
	add_link_options(-fsanitize=address)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmaybe-uninitialized")
set(CMAKE_C_FLAGS_DEBUG "-g -Werror -O0")
set(CMAKE_C_FLAGS_RELEASE "")

# generate the compile_commands for vscode / clang
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# set the project name
project(mod-manager)


set(LIB_SOURCES
	src/lib/steam.c
	src/lib/loadOrder.c
	src/lib/install.c
	src/lib/overlayfs.c
	src/lib/gameData.c
	src/lib/getHome.c
	src/lib/file.c
	src/lib/order.c
	src/lib/archives.c
	src/lib/fomod.c
	src/lib/deploy.c
	src/lib/fomod/group.c
	src/lib/fomod/xmlUtil.c
	src/lib/fomod/parser.c
)

# Headers files to export in the include folder
set(LIB_PUBLIC_HEADERS
	src/include/constants.h
	src/include/deploy.h
	src/include/errorType.h
	src/include/file.h
	src/include/fomod.h
	src/include/gameData.h
	src/include/getHome.h
	src/include/install.h
	src/include/loadOrder.h
	src/include/order.h
	src/include/steam.h
)

set(CLI_SOURCES
	src/cli/cli.c
	src/cli/fomod.c
)

# add the executable
add_executable(mod-manager ${CLI_SOURCES})
add_library(libmod-manager SHARED ${LIB_SOURCES})
set_target_properties(libmod-manager PROPERTIES PUBLIC_HEADER ${LIB_PUBLIC_HEADERS})

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(AUDIT REQUIRED audit)
pkg_search_module(LIBXML REQUIRED libxml-2.0)

target_include_directories(libmod-manager PUBLIC src/include ${GLIB_INCLUDE_DIRS} ${LIBXML_INCLUDE_DIRS} ${AUDIT_INCLUDE_DIRS})
target_link_libraries(libmod-manager ${GLIB_LDFLAGS})
target_link_libraries(libmod-manager ${LIBXML_LIBRARIES})
target_link_libraries(libmod-manager ${AUDIT_LIBRARIES})

target_link_libraries(mod-manager libmod-manager)
target_include_directories(mod-manager PUBLIC src/include)
target_link_libraries(mod-manager ${GLIB_LDFLAGS})
target_include_directories(mod-manager PRIVATE ${GLIB_INCLUDE_DIRS})


install(TARGETS mod-manager OPTIONAL DESTINATION bin)

install(TARGETS libmod-manager
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include/mod-manager
)

set_property(TARGET mod-manager PROPERTY C_STANDARD 23)
set_property(TARGET libmod-manager PROPERTY C_STANDARD 23)

#avoid having liblibmod-manager
set_target_properties(libmod-manager PROPERTIES PREFIX "")
