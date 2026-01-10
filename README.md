
# A work in progress Bethesda game mod manager for Linux

This mod manager is made to protect the original game files files at all costs.
if an operation or a mod tries to modify the files they will be stocked in and overlay folder (~/.local/share/modmanager/UPPER_DIRS/appid)

mods conflicts are solved by priority like in mo3

## Screenshots

![Game list](screenshots/games.png)
![Mods list](screenshots/mods.png)
![Plugins list](screenshots/plugins.png)

## Requirements:
* gtk4
* glib2
* libarchive
* libaudit
* Fuse 3
* xdg-open

## TODO:
1. Find a name
2. Support more games
3. adding a hook for %command% in steam
4. Support morrowind's Morrowind.ini format.

## Supported games:

Skyrim SE is the only currently supported game, others might work and i have made code for them but it's untested.

## How to contribute:
if you have Gamebryo or creation engine based games not listed in subprojects/lib_mod_manager/steam.h
please open an issue with the name of the game folder in steamapps/common/

otherwise don't hesitate to open an issue and suggest changes. if you event know how to code in C don't be scared to create merge request.

## Compilation and install instructions:
download a version from the releases.

then run this inside the folder:
```bash
meson builddir
cd builddir
ninja
```
