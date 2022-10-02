
# A Bethesda game mod manager for Linux

This mod manager is made a protect the game files & mods files at all costs.
if an operation or a mod tries to modify the files they will be stocked in and overlay folder (~/.local/share/modmanager/UPPER_DIRS/appid)

this mod manager will try to enable casefold inside the mods folder as a way of ensuring that there will not be two file with the same name.

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
* A casefold enable filesystem

### Optional requirements:
* Fuse (2 or 3)
* Fuse-overlayfs

## instructions and warnings about casefold:
Casefold is and options in some linux filesystem that will make a folder (not the whole filesystem) case insensitive, this is really good for proton since it gives a small file lookup performance boost and allow us to install mods without fearing having two file with the same name.

sadly i don't think overlayfs support it, but it might still be worth a try. currently i haven't tested without casefold and mods will probably break because of that.



## TODO:
1. we need a name for this.
2. allow to change mod priority.
3. add more game to the code, currently i only have Skyrim se.
4. replace the copy of the games files by hard links if available.
5. try alternatives to overlayfs (searching for casefold support)
6. adding a hook for %command% in steam
7. remake the whole copy function to rename everything in lower case this way we can make casefold obselete.
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

then copy the "ModManager" file to wherever you want. (i recommend ~/.local/bin/)
