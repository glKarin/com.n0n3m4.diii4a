## idTech4A++ (Harmattan Edition)
#### DOOM III/Quake 4/Prey(2006)/DOOM 3 BFG/The Dark Mod for Android/Windows/Linux OpenGLES
#### 毁灭战士3/雷神之锤4/掠食(2006)/毁灭战士3 BFG/The Dark Mod 安卓/Windows/Linux OpenGLES移植版.
##### Original named DIII4A++, based on com.n0n3m4.diii4a's OpenGLES version.
**Latest version:**
1.1.0harmattan52(natasha)  
**Latest update:**
2024-07-17  
**Arch:**
arm64 armv7-a  
**Platform:**
Android 4.4+  
**License:**
GPLv3

----------------------------------------------------------------------------------
### idTech4's feature
* Linux/Windows(MinGW/MSVC(without editor)) build
* multi-threading renderer
* png/dds texture image, jpeg/png/bmp/dds format of screenshot
* obj/dae format static model
* pure soft shadow with shadow-mapping
* soft shadow with stencil-shadow
* OpenGLES2.0/OpenGLES3.0
* OpenAL(soft) and EFX Reverb
* no-lighting rendering and no-lighting material
* translucent stencil shadow
* debug render tools support
* DOOM3(with full body awareness mod)
* Quake4(with bot mod, full body awareness mod) and Raven's idTech4 engine
* Prey(2006)(with full body awareness mod) and HumanHead's idTech4 engine

###### Build DOOM3/Quake4/Prey(2006) mod for Android idTech4A++
[DOOM 3 & RoE SDK](https://github.com/glKarin/idtech4amm_doom3_sdk)  
[Quake 4 SDK](https://github.com/glKarin/idtech4amm_quake4_sdk)  
[Prey(2006) SDK](https://github.com/glKarin/idtech4amm_prey_sdk)  

###### Compare with other OpenGLES rendering version of DOOM3

| Feature                                                                                    |                                       idTech4A++                                       |                            Other                            |
|:-------------------------------------------------------------------------------------------|:--------------------------------------------------------------------------------------:|:-----------------------------------------------------------:|
| Multi-threading                                                                            |                        Support<br/>(but can't switch in gaming)                        | d3es-multithread support<br/>(and support switch in gaming) |
| New stage shader<br/>(heatHaze, heatHazeWithMask, heatHazeWithMaskAndVertex, colorProcess) |                                          Yes                                           |                              -                              |
| TexGen shader                                                                              |                                          Yes                                           |                              -                              |
| Shadow mapping for pure soft shadow                                                        |                                          Yes                                           |                              -                              |
| Translucent stencil shadow                                                                 |                                          Yes                                           |                              -                              |
| Soft stencil shadow                                                                        |                              Yes<br/>(Only OpenGLES3.1+)                               |                              -                              |
| OpenGL ES version                                                                          |            2.0 and 3.0+<br/>(shadow mapping shaders has different version)             |                      2.0(3.0+ compat)                       |
| No lighting                                                                                |                         Yes<br/>(And support switch in gaming)                         |                             Yes                             |
| Debug render tools                                                                         | Yes<br/>(need to set harm_r_renderToolsMultithread to 1 if in multi-threading enabled) |                              -                              |

###### Support games

| Game                         |                             Engine                              |                 Version                 |             OpenGL ES version             |                                                                                         Mods                                                                                         |
|:-----------------------------|:---------------------------------------------------------------:|:---------------------------------------:|:-----------------------------------------:|:-----------------------------------------:|
| DOOM III                     |                         n0n3m4's dante                          |                    -                    |                  2.0/3.0                  | Resurrection of Evil<br/>The Lost Mission<br/>Classic DOOM3<br/>Rivensin<br/>HardCorps<br/>Overthinked Doom^3<br/>Sabot(a7x)<br/>HeXen:Edge of Chaos<br/>Fragging Free<br/>LibreCoop |
| Quake IV                     |                         n0n3m4's dante                          |                    -                    |                  2.0/3.0                  |                                                                                                                                                                                      |
| Prey(2006)                   |                         n0n3m4's dante                          |                    -                    |                  2.0/3.0                  |                                                                                                                                                                                      |
| DOOM 3 BFG(Classic DOOM 1&2) | [RBDOOM-3-BFG](https://github.com/RobertBeckebans/RBDOOM-3-BFG) | 1.4.0<br/>(The last OpenGL renderer version) |                    3.0                    |                                                                                                                                                                                      |
| The Dark Mod                 |             [Dark Mod](https://www.thedarkmod.com)              |                  2.12                   | 3.2<br/>(require geometry shader support) |                                                                                                                                                                                      |
| Return to Castle Wolfenstein |           [iortcw](https://github.com/iortcw/iortcw)            |                    -                    |                    1.1                    |                                           |
| Quake III Arena              |           [ioquake3](https://github.com/ioquake/ioq3)           |                    -                    |                    1.1                    |                                                                                 Quake III Team Arena                                                                                 |
| Quake II                     |      [Yamagi Quake II](https://github.com/yquake2/yquake2)      |                    -                    |                  1.1/3.2                  |                                                                          ctf<br/>rogue<br/>xatrix<br/>zaero                                                                          |
| Quake I                      |  [Darkplaces](https://github.com/DarkPlacesEngine/darkplaces)   |                    -                    |                    2.0                    |                                                                                                                                                                                      |
| GZDOOM                       |        [GZDOOM](https://github.com/ZDoom/gzdoom) 64bits         |                    -                    |                    3.2                    |                                                                                                                                                                                      |

[<img src="https://fdroid.gitlab.io/artwork/badge/get-it-on.png"
     alt="Get it on F-Droid"
     height="80">](https://f-droid.org/packages/com.karin.idTech4Amm/)

Or download the latest APK from the [Releases Section](https://github.com/glKarin/com.n0n3m4.diii4a/releases/latest).
Tag with `-free` only for F-Droid update.

| Feature | Github | F-Droid  |
|:--------|:------:|:--------:|
| Ouya TV |   Yes  |    No    |

----------------------------------------------------------------------------------
### Update

* Add soft stencil shadow support(cvar `harm_r_stencilShadowSoft`) with OpenGLES3.1+ in DOOM3/Quake4/Prey(2006).
* Optimize soft shadow shader with shadow mapping in DOOM3/Quake4/Prey(2006).
* Support r_showSurfaceInfo debug render on multi-threading in DOOM3/Quake4/Prey(2006), need to set cvar `harm_r_renderToolsMultithread` to 1 to enable debug render on multi-threading manually.
* Add GLES3.2 renderer support in Quake2(using +set vid_renderer gles3 for GLES3.2, +set vid_renderer gl1 for GLES1.1).
* Add GZDOOM support on arm64, game data directory named `gzdoom`. More view in [GZDOOM](https://github.com/ZDoom/gzdoom).

----------------------------------------------------------------------------------

* 毁灭战士3/雷神之锤4/掠食(2006)在OpenGLES3.1+下新增模板软阴影(cvar `harm_r_stencilShadowSoft`).
* 毁灭战士3/雷神之锤4/掠食(2006)优化阴影图光照着色器.
* 毁灭战士3/雷神之锤4/掠食(2006)多线程下支持r_showSurfaceInfo调试渲染, 多线程下开启调试渲染工具需要手动设置 cvar `harm_r_renderToolsMultithread`为1才开启.
* 雷神之锤2新增GLES3.2渲染器支持(命令行 GLES3.2渲染器: +set vid_renderer gles3 for GLES3.2, GLES1.1渲染器: +set vid_renderer gl1).
* 新增GZDOOM支持(仅arm64), 游戏数据文件夹为`gzdoom`. 更多详情[GZDOOM](https://github.com/ZDoom/gzdoom).

----------------------------------------------------------------------------------

#### About Prey(2006)
###### For playing Prey(2006)([jmarshall](https://github.com/jmarshall23) 's [PreyDoom](https://github.com/jmarshall23/PreyDoom)). Now can play all levels, but some levels has bugs.
> 1. Putting PC Prey game data file to `preybase` folder and START directly.
> 2. Some problems solution: e.g. using cvar `harm_ui_translateAlienFont` to translate Alien text on GUI.
> 3. Exists bugs: e.g. some incorrect collision(using `noclip`), some GUIs not work(Music CD in RoadHouse).
> 4. If settings UI is not work, can edit `preyconfig.cfg` for binding extras key.
> > * bind "Your key of spirit walk" "_impulse54"
> > * bind "Your key of second mode attack of weapons" "_attackAlt"
> > * bind "Your key of toggle lighter" "_impulse16"
> > * bind "Your key of drop" "_impulse25"

----------------------------------------------------------------------------------

#### About Quake IV
###### For playing Quake 4([jmarshall](https://github.com/jmarshall23) 's [Quake4Doom](https://github.com/jmarshall23/Quake4Doom)). Now can play all levels, but some levels has bugs.  
> 1. Putting PC Quake 4 game data file to `q4base` folder and START directly.
> 2. Suggest to extract Quake 4 patch resource to `q4base` game data folder first(in menu `Other` -> `Extract resource`).
> - `SABot a9 mod` multiplayer-game map aas files and bot scripts(for bots in multiplayer-game).

###### Problems and resolutions  
> 1. *Particle system*: Now is not work(Quake4 using new advanced `BSE` particle system, it not open-source, `jmarshall` has realized and added by decompiling `ETQW`'s BSE binary file, also see [jmarshall23/Quake4BSE](https://github.com/jmarshall23/Quake4BSE)), but it not work yet. Now implementing a OpenBSE with DOOM3 original FX/Particle system, some effects can played, but has incorrect render.
> 2. *Entity render*: Some game entities render incorrect.

###### Bot mod
> 1. Added SABot a7 mod support.
> 2. Extract `q4base/sabot_a9.pk4` file in apk to Quake4 game data folder, it includes some defs, scripts and MP game map AAS file.
> 3. Set cvar `harm_g_autoGenAASFileInMPGame` to 1 for generating a bad AAS file when loading map in Multiplayer-Game and not valid AAS file in current map, you can also put your MP map's AAS file to `maps/mp` folder(botaas32).
> 4. Set `harm_si_autoFillBots` to 1 for automatic fill bots when start MP game.
> 5. Execute `addbots` for add multiplayer bot.
> 6. Execute `fillbots` for auto fill multiplayer bots.

----------------------------------------------------------------------------------
### Screenshot
> Game

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_bathroom.png" alt="Classic bathroom">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_bathroom_jill_stars.png" alt="Classic bathroom in Rivensin mod">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake4.png" alt="Quake IV on DOOM3">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_prey.png" alt="Prey(2006) on DOOM3">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg.jpg" alt="DOOM3 BFG">

> Mod

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_roe.png" width="50%" alt="Resurrection of Evil"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_the_lost_mission.png" width="50%" alt="The lost mission">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_classic_doom3.png" width="50%" alt="Classic DOOM">

> Other

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake_iii_arena.jpg" width="50%" alt="Quake III : Arena"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake_iii_team_arena.jpg" width="50%" alt="Quake III : Team Arena">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_return_to_castle_wolfenstein.jpg" width="50%" alt="Return to Castle Wolfenstein"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_the_dark_mod.jpg" width="50%" alt="The Dark Mod">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake2.jpg" width="50%" alt="Quake II"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake1.jpg" width="50%" alt="Quake I">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom1.jpg" width="33%" alt="DOOM 3 BFG: DOOM I"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom3.jpg" width="34%" alt="DOOM 3 BFG: DOOM III"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom2.jpg" width="33%" alt="DOOM 3 BFG: DOOM II">

----------------------------------------------------------------------------------

### Changes:

[Change logs](CHANGES.md ':include')

----------------------------------------------------------------------------------

### Build:

#### Engine
> 1. _MULTITHREAD: Add multithread support for rendering.
> 2. _USING_STB: Using stb header for jpeg/png/dds texture image and jpeg/png/bmp/dds screenshot support.
> 3. _K_CLANG: If compiling by clang not GCC.
> 4. _MODEL_OBJ: Add obj static model support.
> 5. _MODEL_DAE: Add dae static model support.
> 6. _SHADOW_MAPPING: Add Shadow mapping support.
> 7. _OPENGLES3: Add OpenGLES3.0 support.
> 8. _OPENAL _OPENAL_EFX _OPENAL_SOFT: Add OpenAL(soft) and EFX Reverb support.
> 9. _NO_LIGHT: Add no lighting support.
> 10. _STENCIL_SHADOW_IMPROVE: Add stencil shadow improve support(soft shadow(OpenGLES3.1+), translucent shadow, force combine global shadow and self local shadow).

#### If want to port `Quake4` or `Prey(2006)` to PC or other platform of based on `DOOM3` engine open-source version, because DIII4A based on Android platform and OpenGLES, so has some differences with original version. But I mark some macros in source as patches at all changes, although must find these macros in source code and manual use these patches.
#### And for keeping original DOOM3 source file structures, for all new source files, I put them on a new folder, and in these folder has same directory structure with DOOM3(e.g. framework, renderer, idlib...).

#### Quake 4
##### `_RAVEN`, `_QUAKE4` is patches macros, find them in `DIII4A` source code.
##### All new sources files put on `raven` folder.
> 1. _RAVEN: for compile `core engine (DOOM3 source code)` and `idlib (DOOM3 source code)`.
> 2. _QUAKE4: for compile `game (Q4SDK source code)` library.
> 3. Build core engine: define macro `_RAVEN`, `_RAVEN_FX(OpenBSE if need, unnecessary)`
> 4. Build game library: define macro `_RAVEN`, `_QUAKE4`
##### About `BSE`
Because `BSE` not open-source, so I default supply a `NULL` implement and a uncompleted but working implement with DOOM3 Particle/Fx system(using macros `_RAVEN_FX` marked).
##### About `BOT`
Define macro `MOD_BOTS` will compile SABot a7(from DOOM3) mod source code for bot support in multiplayer-game.
##### About `Full body awareness support`
Define macro `_MOD_FULL_BODY_AWARENESS` will compile Full-body-awareness support.

#### Prey(2006)
##### `_HUMANHEAD`, `_PREY` is patches macros, find them in `DIII4A` source code.
##### All new sources files put on `humanhead` folder.
> 1. _HUMANHEAD: for compile `core engine (DOOM3 source code)` and `idlib (DOOM3 source code)`.
> 2. _PREY: for compile `game (PreySDK source code)` library.
> 3. Build core engine: define macro `_HUMANHEAD`
> 4. Build game library: define macro `_HUMANHEAD`, `_PREY`, and original SDK macros `HUMANHEAD`
##### About `Full body awareness support`
Define macro `_MOD_FULL_BODY_AWARENESS` will compile Full-body-awareness support.

#### Android
##### Define macro `__ANDROID__`.
> 1. _OPENSLES: Add OpenSLES support for sound.

#### Linux
> 1. REQUIRE ALSA, zlib, X11, EGL
> 2. ./cmake_linux_build.sh

#### Windows(MinGW/MSVC)
> 1. REQUIRE SDL2, zlib, cURL
> 2. cmake_msvc_build.bat

----------------------------------------------------------------------------------

### About:

* Source in `assets/source` folder in APK file.
* Using `exportGLSLShaderSource` command can export GLSL shaders.
	
----------------------------------------------------------------------------------

### Branch:

> `master`:
> * /idTech4Amm: frontend source
> * /Q3E /Q3E/src/main/jni/doom3: game source
> * /CHECK_FOR_UPDATE.json: Check for update config JSON

> `free`:
> * For F-Droid pure free version.

> `package`:
> * /screenshot: screenshot pictures
> * /source: Reference source
> * /pak: Game resource

> `n0n3m4_original_old_version`:
> * Original old `n0n3m4` version source.

----------------------------------------------------------------------------------
### Extras download:

* [Baidu网盘: https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w](https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w) 提取码: `pyyj`
* [Baidu贴吧: BEYONDK2000](https://tieba.baidu.com/p/6825594793)
* [F-Droid(different signature)](https://f-droid.org/packages/com.karin.idTech4Amm/)
----------------------------------------------------------------------------------
