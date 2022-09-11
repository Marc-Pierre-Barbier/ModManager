cmake_minimum_required(VERSION 3.10)


# generate the compile_commands for vscode / clang
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# set the project name
project(ModManager)

# add the executable
add_executable(ModManager src/main.c src/steam.c src/install.c src/overlayfs.c src/getHome.c src/file.c)
add_compile_options(-Wall -Wextra -pedantic -Werror)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(audit REQUIRED libaudit)
target_include_directories(ModManager PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(ModManager ${AUDIT_LIBRARIES})

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

target_link_libraries(ModManager ${GLIB_LDFLAGS})
