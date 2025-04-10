#ifndef __MAIN_H__
#define __MAIN_H__

#define MODLIB_NAME "mod-manager"
//no function should ever be put here, only keep important macros and globals here

//relative to home the url is not preprocessed so ../ might create some issues
// in c "A" "B" is the same as "AB"
#define MODLIB_WORKING_DIR ".local/share/" MODLIB_NAME
#define MOD_FOLDER_NAME "MOD_FOLDER"

//the director that will contain all modifications to game files while being deployed
#define GAME_UPPER_DIR_NAME "UPPER_DIRS"
//overlayfs temporary dir.
#define GAME_WORK_DIR_NAME "WORK_DIRS"

//TODO: check if merging them is possible
//file that hold the position in the load order
#define ORDER_FILE "__ORDER__"
//file to describe wether or not we use this mod
#define INSTALLED_FLAG_FILE "__DO_NOT_REMOVE__"

//File that contains the absolute path to the game executables, this allows the user to replace the executable used
#define GAME_EXECUTABLE_FLAG_FILE "__GAME_EXECUTABLE__"

//create a function to get it at runtime
#define MODLIB_VERSION "0.4"
#endif
