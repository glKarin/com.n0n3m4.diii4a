# Changelog
All notable changes to this project will be documented in this file.

## 2021-10-09 - 2021-11-10
- Fixed crash the game when playing the demo with parameter cli_bPrediction=1;
- Fixed attempt to create a udp socket when it is already created
  (the created server stopped responding after the first heartbeat)
- added timeout when reading the response to the master server (setsockopt)
- Increased the CTString PrintF buffer 
  (256 bytes was not enough to form a server response on request \status\,
  look.. CTString.cpp)
- Added \basic\ \info\ \rules\ request
- In GameAgent added support for both TFE and TSE
- In DedicatedServer added support for both TFE and TSE
- Restore green color for TFE (it will be right)
- Enabled building DedicatedServer 
- Added few diagnostic messages
- Fixed bug with loading levels from subdirectories (see IsDirectory() function)
- Added support for 16:9, 16:10, 21:9 screen sides. Fixed menu, NETRICSA, font and HUD for all screen ratios. 
  Fixed the position of the weapon and the player's FOV of view from the first and third person 
  for resolutions 16:9 16:10 21:9 (look 42amsterdam.net fix42 open source mod for a classic sam).
- Added MakeFONT compilation to CMakeLists.txt .
  MakeFONT allows you to create your own fonts for the game.
- Added support for launching mods from the game menu. (look CheckModReload function).
- Deleting using time to get a precision timer for __GNU_INLINE_X86_64__.
  Removing the ugly hack /proc/cpuinfo.
  Adding assembler code to get RDTSC for __GNU_INLINE_X86_64__. 
  Adding several libcpuid functions to get the correct TSC.
  (Note: Using a single RDTSC call in 32-bit mode did not take into account the p-state of the processor, 
  the dynamic frequency of the CPU, and led to different game speeds depending on the current p-state.
  Fixed with a function from libcpuid -  look at Engine\Timer.cpp.
  Using time instead of RDTSC In 64 mode broke the menu, 
  dynamic displays of enemy models and weapons in NETRICSA. Fixed by returning RDTSC.)
- Removed SE1_10.gro because it contains broken textures and breaks the localization of the game. 
  (look the waterfall at the level of the oasis and sierra chiapas). A small SE1_10b.gro is left instead
  containing only the textures of the map for changing the levels of the single player game. 
  The rest of the resources are in the original game files.
- Added variables showing the number of monsters killed, as well as ping for a network game.
  (hud_bShowKills hud_bShowPing).
- Added GetMSLegacyPlayerInf(); function to Entities/Player.es for TFE.
- Added "activemod" to the server response to the request \status\ and \info\.
- Added "difficulty" to the server response to the request \status\ and \rules\.
- Added "location" to the server response to the request \basic\.
  (Note: Now we have full gamespy Implementation of the Query 
  and Reporting without "vipplayers" and "password").
- Removed the day of the week in the strftime() function, 
  as it is incorrectly displayed in the Cyrillic locale.
- Added HUD-Options.cfg and NET-PredictionOptions.cfg in Scripts/CustomOptions.
- Fixed mono sound in MixStereo() funcrioin. Now we have strereo sound in 64-bit mode.
  (look my comments in SoundMixer.cpp and SoundMixer386.asm).
- Fixed broken angle calculation with negative epsilon numbers.
  (see https://github.com/Croteam-official/Serious-Engine/pull/49).
- Fixed loading sequence of tech files in mods.
  (look Engine/Base/Unzip.cpp).
- Fixed Freezing the game when restarting the server at the Hatshepsut level.
  (the bug was in Entities/Lightning.es).

## 2022-02-02 - 2022-02-04
- Added crashfixes for libraries Entities and EntitiesMP from the android port.
  (see https://github.com/Skyrimus/Serious-Sam-Android). 
- Replaced catch char with catch const char. 
- Removed crosshair rendering if the player is dead.

## 2022-05-01 - 2022-05-09
- Fixing sometimes occurring crashes of the game when the player connects to the server.
  (whtn this == null, see Engine/World/World.cpp)
- Added -fno-delete-null-pointer-checks in CMakeLists.txt.
- Added timeout for the connect function when connecting to the master server.
- Added setsockopt for write and read calls when accessing the master server, the socket is switched to non-blocking mode.
- Fixed crash of the game with incorrect cht_iGotoMarker.
- Еngine code excluded from SeriousSam build. Now we have a dynamic libEngine.so library and small SeriousSam,DedicatedServer,MakeFONT files.
  (That's the right thing to do)

## 2022-05-10
- Added amp11lib to project.

## 2022-05-12 - 2022-05-12
- Added Window1251 to Utf8 conversion for SDL_CreateWindow title.
- Added CTString strWindow1251ToUtf8 function.
- Added suport Windows-1251 filenames for game resources.

## 2022-05-23
- Fixed build for FreeBSD. Added definition for FreeBSD.

## 2022-05-24 - 2022-05-25
- Fixed all network issues on FreeBSD.
- Using RDTSC for FreeBSD.

## 2022-06-02 - 2022-06-21
- Fixed build for raspberry pi.
- Fixed broken menu and display of models in NETRICSA for platforms not using RDTSC.
- Reverts Critical Section from win32 original code, set multithreading by default.
- Removed check timestrapped for recorded demo.
- Added definition for raspberry pi and e2k.
- Rewrote the game code. Now all user data is stored in the home directory.

## 2022-07-17 - 2022-11-18
- Structuring paths for installation in /usr/bin /usr/lib /usr/share
- Fixed building for RPI4 on 32-bit system.
- Fixed FreeBSD compilatiun with USE_SINGLE_THREAD=FALSE
- Added game data search.
- Fixed timer for gcc 11.3
- Added hud_bShowScore console variable.
- Fixed old scripts for AMD cards.

## 2022-12-14 - 2022-12-18
- Added support Windows.

## 2022-12-22 - 2022-12-30
- Fixed loading of the amp11lib library for the 64-bit version of the game under Windows.
- BSP,Brushes migration from Float3D to Double3D. Now the WorldEditor is working.
- Made the SeriousSkaStudio work.
- Fixed incorrect x64 cpp-implementation of ASM code in lightmap.
- Fixed 64-bit issues in the World Editor. Now the 64-bit version of the World Editor works.
- Fixed levels load from subdirectory (for Windows).

## 2023-01-02
- Fixed macro redefinition in Float.cpp (Windows only bug). 
  (Note: As a result of redefinition, objects inside the world are located incorrectly.
  For Windows, you need to use the definitions from the header which is in the SDK).

## 2023-02-23 - 2023-03-25
- Fixing a bug with thunder, the sound of thunder and flashes on the level of Hatshepsut.
- Fixed loading libvorbisfile library for non-source-based distros.
- Fixed bug with ioctlwbufget.
- Added portable mode for command line (+portable). In portable mode, all user data is stored in the game directory.

## 2023-03-28 - 2023-05-28
- Added GitGib Action for building artefacts.
- Added optimization for Rasberry PI 3B/4 to CMakeLists.txt.
- Add paths for non-X86 deb based OS to Engine and CMakeLists.txt.
- Fixed building on Fedora 38.
- Fixed incorrect path definition when box 64 is installed for non-x86 systems
- Updated patch links for building demo version.
- Fix to run under Gnome 44 (Gnome 44 bug on sdl2).
- Updated platforms definitions. Added icon and desktop files.
- Fixed install sections. Added icons and desktop files to this sections.
- Updated building scripts.
- Written by Wiki for the game.


## 2023-06-02 - 2023-06-04
- Added search for game libraries, instead of searching for the system path.
- Added 5:4 aspect ratio, 8k resolution.
- Added build for macOS and build in Github Action.
- Forced mdl_bFineQuality=1 because otherwise weapon models break.
- Forcing shd_iDynamicSize to be set to shd_iStaticSize, otherwise entire sectors may be highlighted when fire shooting.

## 2023-06-07 - 2023-06-09
- Adding root CMakeLists.txt to build files for Serious Sam Classic The First Encounter and Serious Sam Classic The Second Encounter.
- Added debian directory for building deb package using regular tools(bzr debuild) under Debian/Ubuntu.

## 2023-06-15 - 2023-07-07
- Fixed OpenGL lens flares,fix platform definition and CMakeList.
- Fixed loading of some incorrect custom maps.

## 2023-10-12 - 2023-10-14
- Fixed Obelisk textures and Alley Of Sphinxes textures (TFE).
- Fixed incorrect variable type in Beast.es.
- Removed deprecated PlatformToolset from aml11lib (Win).
- Fixed Serious Editor 64-bit crash when calling the function to show all entities <N>.
  Now the 64-bit editor has full functionality, just like its 32-bit version.
  The cause of the crash was the conversion of pointers to 32 bits. 
- SeriousModeler: Fixed conversion of pointers to 32 bits (Win).
- SeriousSkaStudio: Fixed conversion of pointers to 32 bits (Win).
- Fixed HUD, player FOV, weapon FOV for Serious Editor.
- Restored name for Serious Editor log file.
- Fixed some textures after Save/Load (First Encounter).
- Added an additional method to fix the glow of textures.
  Now you can select the fixation method. You can choose to fix the parameters
  of textures that emit a illumination, or a method that creates additional light sources
  that cancel the texture illumination bug.Both methods dynamically edit the world on the
  fly after the level is loaded, before the game starts. Loads/Saves, as well as 
  Demo Recordings (including those made earlier) are now fixded too.
  This fix affects the following levels: "Intro", "Karnak Demo", "Valley Of TheKings",
  "Dunes", "Suburbs", "Metropolis", "Alley Of Sphinxes", "Karnak", "Luxor", "Sacred Yards".
  Other levels do not have this bug.

## 2023-10-20 - 2023-10-21
- Fixed non-rendering of textures on some levels. See fix https://github.com/tx00100xt/SeriousSamClassic/issues/19
- Fixed a bug in multiplayer when changing levels. See fix https://github.com/tx00100xt/SeriousSamClassic/issues/20
- Add hud_bWeaponsIconScale var for scale HUD weapons icon.
- Large weapon icons are set by default.
- Added new Scripts and updated other.
- Additional light sources for XPLUS also.
- Restored the original menu design as in TFE (grid, clouds and so on).

## 2023-11-07 - 2023-11-28
- Fix Debug install sections.
- Fixed delay in LimitFrameRate function.
  Look here: https://forum.ubuntu.ru/index.php?topic=320015.msg2489163#msg2489163
- Added OpenBSD and NetBSD support.
- Added install to system for FreeBSD, OpenBSD and NetBSD.
- Added templates for FreeBSD, OpenBSD and NetBSD for build package.
- Made many corrections in the code regarding compiler warnings.
- Added man pages for.for FreeBSD, OpenBSD and NetBSD.
- Added searching library libvorbisfile for BSD platforms.

## 2023-12-01 - 2023-12-26
- Add building portable version for x86 and amd64 without asm code.
- Fixed creation of textures from TGA images in the Modeler and Editor.
- Added man pages Man pages for Gentoo.
- Fixed broken water and fire effects for textures. They were broken when ported to Linux in 2016.
- Adding the TEXConv utility that converts normal textures into TGA images.
