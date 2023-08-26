cmake_minimum_required(VERSION 3.6)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-DDEBUG)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmaybe-uninitialized -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_C_STANDARD 23)
# generate the compile_commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

project(mod-manager)

#############
#  SOURCES  #
#############

set(LIB_SOURCES
	src/lib/steam.c
	src/lib/loadOrder.c
	src/lib/overlayfs.c
	src/lib/gameData.c
	src/lib/getHome.c
	src/lib/file.c
	src/lib/mods.c
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
	src/include/loadOrder.h
	src/include/mods.h
	src/include/steam.h
)

set(CLI_SOURCES
	src/cli/cli.c
	src/cli/fomod.c
)

set(GUI_SOURCES
	src/gui/main.c
	src/gui/game_tab.c
	src/gui/fomod_install.c
	src/gui/mod_tab.c
	src/gui/plugin_tab.c
)

#####################
#  BINARIES CONFIG  #
#####################

# add the executable
add_executable(mod-manager-cli ${CLI_SOURCES})
add_executable(mod-manager-gui ${GUI_SOURCES})
add_library(libmod-manager SHARED ${LIB_SOURCES})

#########################
#    DEPENDENCIES       #
#########################
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_search_module(AUDIT REQUIRED IMPORTED_TARGET audit)
pkg_search_module(LIBXML REQUIRED IMPORTED_TARGET libxml-2.0)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(ADWAIDA REQUIRED IMPORTED_TARGET libadwaita-1)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

# LIB
target_link_libraries(libmod-manager PUBLIC PkgConfig::GLIB PkgConfig::LIBXML PkgConfig::AUDIT PkgConfig::GIO)

# CLI
target_include_directories(mod-manager-cli PUBLIC src/include)
target_link_libraries(mod-manager-cli PUBLIC libmod-manager PkgConfig::GLIB PkgConfig::GIO)

# GUI
target_include_directories(mod-manager-gui PUBLIC src/include)
target_link_libraries(mod-manager-gui PUBLIC libmod-manager PkgConfig::GTK4 PkgConfig::GLIB PkgConfig::ADWAIDA PkgConfig::GIO)

#####################
#  INSTALL OPTIONS  #
#####################

install(TARGETS mod-manager-cli OPTIONAL DESTINATION bin)
install(TARGETS mod-manager-gui OPTIONAL DESTINATION bin)

target_sources(libmod-manager PUBLIC FILE_SET HEADERS
	BASE_DIRS src/include/
	FILES ${LIB_PUBLIC_HEADERS}
)

install(TARGETS libmod-manager
	LIBRARY DESTINATION lib
	FILE_SET HEADERS DESTINATION include/mod-manager
)

#avoid having liblibmod-manager
set_target_properties(libmod-manager PROPERTIES PREFIX "")
