#ifndef __MAIN_H__
#define __MAIN_H__

#define MODLIB_NAME "mod-manager"
//no function should ever be put here, only keep important macros and globals here

//relative to home the url is not preprocessed so ../ might create some issues
// in c "A" "B" is the same as "AB"
#define MODLIB_WORKING_DIR ".local/share/" MODLIB_NAME
#define MOD_FOLDER_NAME "MOD_FOLDER"
//the folder in which the games file will be linked or copied
#define GAME_FOLDER_NAME "GAME_FOLDER"
//the director that will contain all modifications to game files while being deployed
#define GAME_UPPER_DIR_NAME "UPPER_DIRS"
//overlayfs temporary dir.
#define GAME_WORK_DIR_NAME "WORK_DIRS"

//create a function to get it at runtime
#define MODLIB_VERSION "0.3"
#endif
