PARTYMOD THAW 0.10

This is a patch for THAW to improve its input handling as well as smooth out a few other parts of the PC port.
The patch is designed to keep the game as original as possible, and leave its files unmodified.

Features and Fixes:
- Replaced input system entirely with new, modern system using the SDL2 library
- Improved window handling allowing for custom resolutions and configurable windowing
- Replaced configuration files with new INI-based system (see partymod.ini)
- Custom configurator program to handle new configuration files
- Fixes an issue with the level select where it sometimes doesn't appear

INSTALLATION:
1. Make sure THAW (English) is installed and the 1.01 patch is applied, remove the widescreen mod if it is installed (delete dinput8.dll)
2. Extract this zip folder into your THAW installation `game/` directory
3. Run partypatcher.exe to create the new, patched THAWPM.exe game executable (this will be used to launch the game from now on) (this only needs to be done once)
4. Optionally (highly recommended), configure the game with partyconfig.exe
5. Launch the game from THAWPM.exe

NOTE: if the game is installed into the "Program Files" directory, you may need to run each program as administrator. 
Also, if the game is installed into the "Program Files" directory, save files will be saved in the C:\Users\<name>\AppData\Local\VirtualStore directory.  
For more information, see here: https://answers.microsoft.com/en-us/windows/forum/all/please-explain-virtualstore-for-non-experts/d8912f80-b275-48d7-9ff3-9e9878954227