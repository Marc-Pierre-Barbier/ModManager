
# A Bethesda game mod manager for Linux

This mod manager is made a protect the game files & mods files at all costs.
if an operation or a mod tries to modify the files they will be stocked in and overlay folder (~/.local/share/modmanager/UPPER_DIRS/appid)

mods conflicts are solved by priority, currently we still have to implements
a way to change the priority.


```
Use --list-games or -l to list compatible games
Use --add or -a <APPID> <FILENAME> to add a mod to a game
Use --list-mods or -m <APPID> to list all mods for a game
Use --install or -i <APPID> <MODID> to add a mod to a game
Use --uninstall or -u <APPID> <MODID> to uninstall a mod from a game
Use --remove or -r <APPID> <MODID> to remove a mod
Use --deploy or -d <APPID> to deploy the mods for the game
Use --unbind <APPID> to rollback a deployment
Use --fomod <APPID> <MODID> to create a new mod using the result from the FOMod
```

## Requirements:
* glib2
* 7z unzip unrar

### Optional requirements:
* Fuse (2 or 3)
* Fuse-overlayfs

## TODO:
1. we need a name for this.
2. allow to change mod priority.
3. add more game to the code, currently i only have Skyrim se.
6. adding a hook for %command% in steam
8. check if the order of the overlayfs directories is the right one.

## How to contribute:
if you have creation engine based games not listed in src/steam.h
please open an issue with the EXACT name of the game folder in steamapps/common/

otherwise if you know how to code in c don't be scared to create mr or suggest changes.

don't try to make me change the licence i chose GPL-2 on purpose and i won't use GPL-3.

## Compilation and install instructions:
download a version from the releases(please avoir a simple clone of the git).

then run this inside the folder:
```bash
mkdir build
cd build
cmake ..
make
```

then copy the "mod-manager" file to wherever you want. (i recommend ~/.local/bin/)
