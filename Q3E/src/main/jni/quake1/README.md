# DarkPlaces Engine

DarkPlaces is a game engine based on the Quake 1 engine by id Software. It
improves and builds upon the original 1996 engine by adding modern rendering
features, and expanding upon the engine's native game code language QuakeC, as
well as supporting additional map and model formats.

Developed by LadyHavoc. See [CREDITS](CREDITS.md) for a list of contributors.

## Help/support

### IRC
#darkplaces on irc.anynet.org

### [Matrix](https://matrix.org/docs/guides/introduction)
[![#darkplaces:matrix.org](https://img.shields.io/matrix/darkplaces:matrix.org?color=660000&label=%23darkplaces%3Amatrix.org)](https://matrix.to/#/#darkplaces:matrix.org)

## Downloading and running

Linux x86_64 builds are available in [GitHub CI](https://github.com/DarkPlacesEngine/darkplaces/actions?query=branch%3Amaster) artifacts.  

More complete builds are available in [xonotic.org](https://beta.xonotic.org/autobuild/) engine zips.  
These support Windows, Linux and macOS, and include the current libraries needed for all features.

DarkPlaces supports many Quake-based games and you can select which it will run by renaming the executable so it's prefixed with the game's name, for example `rogue-sdl.exe`, or by passing a cmdline argument  
such as `-rogue`.  This changes various engine behaviours and cvar defaults to suit the game.  
The supported list and related details are defined in [com_game.c](https://github.com/DarkPlacesEngine/darkplaces/blob/master/com_game.c).

Mods which aren't listed there can be run with (for example) `-game quake15` in which case DP will use the same behaviours and cvar defaults as for id1 Quake.

## Quake Virtual File System

All of Quake's data access is through a hierarchical file system, the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and
all game directories.  This can be overridden with the "-basedir" command
line parm to allow code debugging in a different directory.  The base
directory is only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that
all generated files (savegames, screenshots, demos, config files) will be
saved to.  This can be overridden with the "-game" command line parameter.
If multiple "-game <gamedir>" args are passed the last one is the "primary"
and files will be saved there, the rest are read-only.

## Build instructions (WIP)

These instructions are adequate for Quake, but for Xonotic please refer to [its wiki](https://gitlab.com/xonotic/xonotic/-/wikis/Compiling).

### Required packages

The minimum SDL version is 2.0.18 for Linux and 2.24.0 for Windows.  
The supported compilers are GCC and Clang.  
The following package names are for Debian, see below for Windows and Mac.

##### Client
Build (mandatory): `build-essential` `libjpeg-dev` `libsdl2-dev`  
Runtime (optional): `libcurl` `libpng` `libfreetype6` `libvorbisfile`  

##### Dedicated Server
Build (mandatory): `build-essential` `libjpeg-dev` `zlib1g-dev`  
Runtime (optional): `libcurl` `libpng`  

### Windows (MSYS2 MinGW):

1. Install MSYS2, found [here](https://www.msys2.org/).
2. Once you've installed MSYS2 and have fully updated it, open a MinGW64 terminal (***not an MSYS2 terminal***) and input the following command:

```
pacman -S --needed gcc make mingw-w64-x86_64-{toolchain,libjpeg-turbo,libpng,libogg,libvorbis,SDL2}
```

3. See [Unix instructions](#unix-(general)).

### macOS
1. Open a terminal and input `xcode-select --install`
2. Install [Homebrew](https://brew.sh)
3. In the same (or a different terminal), input the following command:

```
brew install sdl2 libjpeg-turbo libpng libvorbis curl
```

4. See [Unix instructions](#unix-(general)).

### Unix (General)

From a terminal, in the engine's root directory, input `make help` to list the targets.  
To build the main executable, input `make sdl-release` which creates the file called  
`darkplaces-sdl` or `darkplaces-sdl.exe` (Windows).

If you get errors (that don't seem to be about missing dependencies) try `make clean` before compiling, especially if you updated your system since the last time you compiled.


### Windows (Visual Studio 2019)

Not recommended due to poor support for C standards, and lack of maintenance.

DarkPlaces requires C11, so Windows SDK 10.0.20348.0 or later is needed.  
To install it, run the Visual Studio Installer, click "Modify", click "Individual components", type "Windows SDK" in the search box, select the latest Windows SDK and de-select older versions.  
You will also need "NuGet package manager" selected (to download SDL2 headers the first time you build).
Click "Modify" to apply the changes.  

Open `darkplaces-vs2019.sln`, select build type (`Debug` or `Release`) and platform (`Win32` or `x64`), and choose "Build Solution" from the "Build" menu to create files `darkplaces-sdl2-vs2019.exe` and `SDL2.dll`.

The Release build crashes. The Debug x64 build doesn't crash (but is rather slow) so this will be Fun for someone to debug.

To get a build suitable for playing you'll need to use MinGW GCC, or download the autobuild from Xonotic (see above).


### Web-Assembly (Emscripten)

Note that this requires a linux device or WSL2.

1. Install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions-using-the-emsdk-recommended)
1. After activating and sourcing emsdk, compile DarkPlaces for wasm using;
   ```shell
   make emscripten-release
   ```
1. Copy `darkplaces-wasm.js`, `wasm/index.html`, and `wasm/autoexec.cfg` files to your web server
1. Copy the Quake `pak0.pak` and any other files into the same web server directory

For the standalone version (single HTML file containing engine and data):
1. Before compiling, copy game data and .cfg files to the appropriate gamedir in `wasm/preload` (for example, pak0 from Quake would be in `wasm/preload/id1/pak0.pak`)
1. After activating and sourcing emsdk, compile DarkPlaces for wasm using;
   ```shell
   make emscripten-standalone
   ```
1. To start DP you must click somewhere in the window!
1. If you want to upload files into the game filesystem, use `em_upload` in the darkplaces console (upload to /save if you want it to save across restarts)
1. To save the stuff you uploaded to /save, use `em_save` (note that if you embedded the game, you won't be able to save changes to `/save/games`)


## Contributing

[DarkPlaces Contributing Guidelines](CONTRIBUTING.md)

## Documentation

Doxygen: https://xonotic.org/doxygen/darkplaces
