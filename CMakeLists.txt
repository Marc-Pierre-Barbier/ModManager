cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
	add_link_options(-fsanitize=address)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmaybe-uninitialized")
set(CMAKE_C_FLAGS_DEBUG "-g -fsanitize=address -Werror -O0")
set(CMAKE_C_FLAGS_RELEASE "-g")

# generate the compile_commands for vscode / clang
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# set the project name
project(mod-manager)


set(SOURCES
	src/steam.c
	src/loadOrder.c
	src/install.c
	src/overlayfs.c
	src/getDataPath.c
	src/getHome.c
	src/file.c
	src/order.c
	src/archives.c
	src/fomod.c
	src/fomod/group.c
	src/fomod/xmlUtil.c
	src/fomod/parser.c
)

# add the executable
add_executable(mod-manager src/main.c ${SOURCES})

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(AUDIT REQUIRED audit)
pkg_search_module(LIBXML REQUIRED libxml-2.0)

target_include_directories(mod-manager PRIVATE ${GLIB_INCLUDE_DIRS} ${LIBXML_INCLUDE_DIRS} ${AUDIT_INCLUDE_DIRS})


target_link_libraries(mod-manager ${GLIB_LDFLAGS})
target_link_libraries(mod-manager ${LIBXML_LIBRARIES})
target_link_libraries(mod-manager ${AUDIT_LIBRARIES})

install(TARGETS mod-manager DESTINATION bin)

set_property(TARGET mod-manager PROPERTY C_STANDARD 23)
