//no function should ever be put here, only keep important macros here

#define APP_NAME "modmanager"
//relative to home the url is not preprocessed so ../ might create some issues
// in c "A" "B" is the same as "AB"
#define MANAGER_FILES ".local/share/" APP_NAME
#define MOD_FOLDER_NAME "MOD_FOLDER"
//the folder in which the games file will be linked or copied
#define GAME_FOLDER_NAME "GAME_FOLDER"
//the director that will contain all modifications to game files while being deployed
#define GAME_UPPER_DIR_NAME "UPPER_DIRS"
//overlayfs temporary dir.
#define GAME_WORK_DIR_NAME "WORK_DIRS"
