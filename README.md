
# A Bethesda game mod manager for Linux

This mod manager is made to protect the original game files files at all costs.
if an operation or a mod tries to modify the files they will be stocked in and overlay folder (~/.local/share/modmanager/UPPER_DIRS/appid)

mods conflicts are solved by priority like in mo3


```
Use --list-games or -l to list compatible games
Use --setup <APPID> to configure the game for the first time (will take a lot of storage in /home if the game is not in /home)

Use --bind or -d <APPID> to deploy the mods for the game
Use --unbind <APPID> to rollback a deployment

Use --add or -a <APPID> <FILENAME> to add a mod to a game
Use --remove or -r <APPID> <MODID> to remove a mod
Use --fomod <APPID> <MODID> to create a new mod using the result from the FOMod
Use --list-mods or -m <APPID> to list all mods for a game
Use --install or -i <APPID> <MODID> to add a mod to a game
Use --uninstall or -u <APPID> <MODID> to uninstall a mod from a game
Use --swap <APPID> <MODID A> MODID B> to swap two mod in the loading order
Use --version or -v to get the version number
Use --show-load-order <APPID>
Use --list-plugins <APPID>
```

## Requirements:
* glib2
* 7z unzip unrar (used for each respective formats)
* libaudit

### Optional requirements:
* Fuse (2 or 3)
* Fuse-overlayfs

## TODO:
1. Find a name
2. add more game to the code, currently i only have Skyrim se.
3. adding a hook for %command% in steam
4. Code a GUI

## How to contribute:
if you have creation engine based games not listed in src/steam.h
please open an issue with the name of the game folder in steamapps/common/

otherwise don't hesitate to open an issue and suggest changes. if you event know how to code in C don't be scared to create merge request.

## Note on the licence
i chosed gpl2 as the only thing i want is that if i give you code you give me your changes back.

## Compilation and install instructions:
download a version from the releases.

then run this inside the folder:
```bash
mkdir build
cd build
cmake ..
make
```

then copy the "mod-manager" file to wherever you want. (i recommend ~/.local/bin/)
