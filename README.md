# PARTYMOD for THAW
This is a patch for THAW 1.01 to improve its input handling as well as smooth out a few other parts of the PC port.
The patch is designed to keep the game as original as possible, and leave its files unmodified.

This patch was originally developed to provide input patches to be integrated into reTHAWed.  Thanks to the reTHAWed team (especially uzis and trxbail) for some assistance and inspiring this patch!

### Features and Fixes
* Replaced input system entirely with new, modern system using the SDL2 library
* Improved window handling allowing for custom resolutions and configurable windowing
* Replaced configuration files with new INI-based system (see partymod.ini)
* Custom configurator program to handle new configuration files
* Fixes an issue with the level select where it sometimes doesn't appear
* Fixes hanging on quitting to menu (without delaying level loading!)
* Adds entropy to music shuffling to make sure it doesn't repeat between sessions (while consuming the same number of random numbers)
* Fixes camera precision loss at great distances (the camera shake noticeable at the far ends of the world, like Oil Rig)

### Installation
1. Download PARTYMOD from the releases tab
2. Make sure THAW (English) is installed and the 1.01 patch is applied, remove the widescreen mod if it is installed (delete dinput8.dll)
3. Extract this zip folder into your THAW installation `game/` directory
4. Run partypatcher.exe to create the new, patched THAWPM.exe game executable (this will be used to launch the game from now on) (this only needs to be done once)
5. Optionally (highly recommended), configure the game with partyconfig.exe
6. Launch the game from THAWPM.exe

NOTE: when the game runs, it will generate a scriptCache.qbc file.  Once this cache is fully generated (once groundtricks.qb and levelselect_scripts.qb are patched), the game will rely on this for those patched files, which allows you to modify other scripts more easily.

NOTE: if the game is installed into the "Program Files" directory, you may need to run each program as administrator. 
Also, if the game is installed into the "Program Files" directory, save files will be saved in the C:\Users\<name>\AppData\Local\VirtualStore directory.  
For more information, see here: https://answers.microsoft.com/en-us/windows/forum/all/please-explain-virtualstore-for-non-experts/d8912f80-b275-48d7-9ff3-9e9878954227

### Building
The build requires CMake and SDL2 (I install it via vspkg).  Create the project file like so from the partymod-thps3/build directory:
```
cmake .. -A win32 -DCMAKE_TOOLCHAIN_FILE=C:/[vcpkg directory]/scripts/buildsystems/vcpkg.cmake
```

Set the optimization optimization for the partymod dll to O0 (disable optimization) because MSVC seems to break certain functions when optimization is enabled.
Additionally, set the SubSystem to "Windows (/SUBSYSTEM:WINDOWS)" in the partyconfig project.

### TODO
Some stuff I would like to do at some point
* Find a way to deal with Securom protected executables
* Figure out how to generically patch scripts