# idTech4A++ (Harmattan Edition)  <img align="right" width="128" height="128" src="https://github.com//glKarin/com.n0n3m4.diii4a/raw/master/idTech4Amm/src/main/res/drawable/icon.png" alt="idTech4A++" />  
[![Android Build Actions Status](https://github.com/glKarin/com.n0n3m4.diii4a/actions/workflows/android.yml/badge.svg)](https://github.com/glKarin/com.n0n3m4.diii4a/actions/workflows/android.yml) [![Windows/Linux Build Actions Status](https://github.com/glKarin/com.n0n3m4.diii4a/actions/workflows/win_linux.yml/badge.svg)](https://github.com/glKarin/com.n0n3m4.diii4a/actions/workflows/win_linux.yml)  
[![Discord chat](https://img.shields.io/discord/1398154850239254568.svg?logo=discord&label=Discord%20chat)](https://discord.gg/KFshBra4kh)  
[![Latest Release](https://img.shields.io/github/downloads/glKarin/com.n0n3m4.diii4a/latest/total)](https://github.com/glKarin/com.n0n3m4.diii4a/releases)  
[![Download Android testing](https://img.shields.io/github/downloads/glKarin/com.n0n3m4.diii4a/android_testing/total?label=downloads%40Android%20testing
)](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/android_testing) [![Download Windows/Linux testing](https://img.shields.io/github/downloads/glKarin/com.n0n3m4.diii4a/win_linux_testing/total?label=downloads%40Windows%2FLinux%20testing
)](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/win_linux_testing)  

[<img src="https://fdroid.gitlab.io/artwork/badge/get-it-on.png"
     alt="Get it on F-Droid"
     height="80">](https://f-droid.org/packages/com.karin.idTech4Amm/)

**idTech** engine games **For** **A**ndroid. An **idTech** games runtime libraries collection on Android  
#### DOOM III/Quake 4/Prey(2006) OpenGLES on Android/Windows/Linux  
##### DOOM 3 BFG/The Dark Mod/Quake 1 2 3/RTCW/GZDOOM/ETW/RealRTCW/FTEQW/STAR WARS™ Jedi Knight/Serious Sam Classic/Urban Terror/OpenMOHAA/Skin Deep on Android   
##### Original named DIII4A++, based on com.n0n3m4.diii4a's OpenGLES version.
**Latest version:**
1.1.0harmattan71(whip)  
**Latest update:**
2026-02-10  
**Arch:**
arm64 armv7-a  
**Platform:**
Android 4.4+  
**License:**
GPLv3  

----------------------------------------------------------------------------------

> #### idTech4's feature 
* Linux/Windows(MinGW/MSVC(without editor)) build
* multi-threading renderer
* pure soft shadow with shadow-mapping
* soft shadow with stencil-shadow and translucent stencil shadow
* global illumination rendering
* lighting model: Phong/Blinn-phong/PBR/Ambient/No-lighting
* Wide-character language translation and BFG new fonts support
* debug render tools support with programming render pipeline
* OpenGLES2.0/OpenGLES3.0
* png/dds/bimage texture image, jpeg/png/bmp/dds format of screenshot
* obj/dae/md5mesh format static model
* psk&psa/iqm/gltf&glb/fbx format animation/static model
* OpenAL(soft) and EFX Reverb
* Float console
* DOOM3(with full body awareness mod, view body mod, bot mod, view flashlight mod)
* Quake4(with bot mod, full body awareness mod, view body mod) and Raven's idTech4 engine
* Prey(2006)(with full body awareness mod) and HumanHead's idTech4 engine

----------------------------------------------------------------------------------

> #### Support games/mods

| Game | Engine | Version | OpenGL ES version | Vulkan version | Standalone folder<br/>(* means always enabled) | Mods/Plugins |
|:-----|:-----:|:-----:|:-----:|:-----:|:-----:|:-----|
| DOOM III | n0n3m4's dante | - | 2.0/3.0 |  | doom3 | [Resurrection of Evil]()<br/>[The Lost Mission](https://www.moddb.com/mods/the-lost-mission)<br/>[Classic DOOM3](https://www.moddb.com/mods/classic-doom-3)<br/>[Rivensin](https://www.moddb.com/mods/ruiner)<br/>[HardCorps](https://www.moddb.com/mods/hardcorps)<br/>[Overthinked Doom^3](https://www.moddb.com/mods/overthinked-doom3)<br/>[Sabot(a7x)](https://www.moddb.com/games/doom-3-resurrection-of-evil/downloads/sabot-alpha-7x)<br/>[HeXen:Edge of Chaos](https://www.moddb.com/mods/hexen-edge-of-chaos)<br/>[Fragging Free](https://www.moddb.com/mods/fragging-free)<br/>[LibreCoop](https://www.moddb.com/mods/librecoop-dhewm3-coop)<br/>[LibreCoop D3XP](https://www.moddb.com/mods/librecoop-dhewm3-coop)<br/>[Perfected Doom 3](https://www.moddb.com/mods/perfected-doom-3-version-500)<br/>[Perfected Doom 3:RoE](https://www.moddb.com/mods/perfected-doom-3-version-500)<br/>[Doom 3: Phobos](https://www.moddb.com/mods/phobos)([Dhewm3 compatibility patch](https://www.moddb.com/games/doom-iii/addons/doom-3-phobos-dhewm3-compatibility-patch)) |
| Quake IV | n0n3m4's dante | - | 2.0/3.0 |  | quake4 | [Hardqore](https://www.moddb.com/mods/quake-4-hardqore) |
| Prey(2006) | n0n3m4's dante | - | 2.0/3.0 |  | prey |  |
| DOOM 3 BFG(Classic DOOM 1&2) | [RBDOOM-3-BFG](https://github.com/RobertBeckebans/RBDOOM-3-BFG) | 1.4.0<br/>(The last OpenGL renderer version) | 3.0 | 1.1 | doom3bfg |  |
| The Dark Mod | [Dark Mod](https://www.thedarkmod.com) | 2.13 | 3.2<br/>(require geometry shader support) |  | darkmod * |  |
| Return to Castle Wolfenstein | [iortcw](https://github.com/iortcw/iortcw) | 1.51d | 1.1 |  | rtcw |  |
| Quake III Arena | [ioquake3](https://github.com/ioquake/ioq3) | 1.36 | 1.1 |  | quake3 | Quake III Team Arena |
| Quake II | [Yamagi Quake II](https://github.com/yquake2/yquake2) | 8.60 | 1.1/3.2 | 1.0 | quake2 | Capture The Flag<br/>Ground Zero<br/>The Reckoning<br/>Team Evolves Zaero<br/>3rd Zigock Bot II |
| Quake I | [Darkplaces](https://github.com/DarkPlacesEngine/darkplaces) | - | 2.0 | |  quake1 |  |
| UZDOOM(64bits) | [UZDOOM](https://github.com/UZDoom/uzdoom) | 4.14.2 | 2.0/3.2 | 1.0 | uzdoom * |  |
| Wolfenstein: Enemy Territory | [ET: Legacy](https://www.etlegacy.com) | 2.83.2 | 1.1 |  | etw |  Omni-Bot support |
| RealRTCW | [RealRTCW](https://github.com/wolfetplayer/RealRTCW) | 5.3 | 1.1 |  | realrtcw |  |
| STAR WARS™ Jedi Knight - Jedi Academy™ | [OpenJK](https://github.com/JACoders/OpenJK) | 1.0.1.1 | 1.1 |  | openja |  |
| STAR WARS™ Jedi Knight II - Jedi Outcast™ | [OpenJK](https://github.com/JACoders/OpenJK) | 1.0.1.1 | 1.1 |  | openjo |  |
-- | FTEQW | [FTEQW](https://www.fteqw.org) | 1.05 | 3.2 | 1.1 | fteqw * |  |
| Serious Sam Classic : The First Encounter | [SamTFE](https://github.com/tx00100xt/SeriousSamClassic) | 1.10.7 | 1.1 |  | serioussamtfe * |  |
| Serious Sam Classic : The Second Encounter | [SamTSE](https://github.com/tx00100xt/SeriousSamClassic) | 1.10.7 | 1.1 |  | serioussamtse * |  |
| Urban Terror | [Q3-UT4](https://www.urbanterror.info) | 4.3.4 | 1.1 |  | urt |  |
| Medal of Honor: Allied Assault | [OpenMOHAA](https://github.com/openmoh/openmohaa) | 0.82.1 | 1.1 |  | openmohaa |  |
| Skin Deep | [SkinDeep](https://blendogames.com/skindeep/) | 1.0.4 | 3.2 |  | skindeep |  |

----------------------------------------------------------------------------------

> #### Update 1.1.0harmattan71 (2026-02-10)

* Add `Skin Deep`(ver 1.0.4) support, game standalone directory named `skindeep`, game data directory named `base`.
* Support edit on-screen buttons layout in gaming.
* Add `ECWolf`(ver 1.4.2) support, game standalone directory named `ecwolf`.
* Add game main thread stack size config on Menu > Option > Advance.
* Enable `FTEQW`.
* Add `UZDoom`(ver 4.14.3) arm64 support, game standalone directory named `uzdoom`, And GZDoom is removed.
* Fix some GUIs in Quake 4/Prey(2006).
* RealRTCW(ver 5.2) is removed.

----------------------------------------------------------------------------------

> #### idTech4's new Cvar/Commands:

[Cvar list](doc/idTech4_new_cvars ':include')  
[Command list](doc/idTech4_new_commands ':include')

----------------------------------------------------------------------------------

> #### About DOOM 3
##### Switch current weapon and last weapon  
```bind "Your key" "_impulse51"```

----------------------------------------------------------------------------------

> #### About Quake IV
##### For playing Quake 4([jmarshall](https://github.com/jmarshall23) 's [Quake4Doom](https://github.com/jmarshall23/Quake4Doom)). Now can play all levels, but some levels has bugs.  
1. Putting PC Quake 4 game data file to `q4base` folder and START directly.
2. *Effect system*: Quake4 new advanced `BSE` particle system is working now! Also see [Quake4BSE](https://github.com/jmarshall23/Quake4BSE), [Quake4Decompiled](https://github.com/jmarshall23/Quake4Decompiled), and OpenBSE with DOOM3 original FX/Particle system has been removed.

----------------------------------------------------------------------------------

> #### About Prey(2006)
##### For playing Prey(2006)([jmarshall](https://github.com/jmarshall23) 's [PreyDoom](https://github.com/jmarshall23/PreyDoom)). Now can play all levels, but some levels has bugs.
1. Putting PC Prey game data file to `base`(`preybase` on Android) folder and START directly.
2. Some problems solution: e.g. using cvar `harm_ui_translateAlienFont` to translate Alien text on GUI.
3. Exists bugs: e.g. some incorrect collision(using `noclip`), some GUIs not work(Music CD in RoadHouse).
4. If settings UI is not work, can edit `preyconfig.cfg` for binding extras key.
```
bind "Your key of spirit walk" "_impulse54"
bind "Your key of second mode attack of weapons" "_attackAlt"
bind "Your key of toggle lighter" "_impulse16"
bind "Your key of drop" "_impulse25"
```

----------------------------------------------------------------------------------

> #### Screenshot
##### Game

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_bathroom.png" width="33%" alt="Classic bathroom"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake4.png" width="33%" alt="Quake IV on DOOM3"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_prey.png" width="33%" alt="Prey(2006) on DOOM3">

##### Mod

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_roe.png" width="33%" alt="Resurrection of Evil"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_the_lost_mission.png" width="33%" alt="The lost mission"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_classic_doom3.png" width="33%" alt="Classic DOOM">

##### Other

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom1.jpg" width="33%" alt="DOOM 3 BFG: DOOM I"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom3.jpg" width="33%" alt="DOOM 3 BFG: DOOM III"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom2.jpg" width="33%" alt="DOOM 3 BFG: DOOM II">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake_iii_arena.jpg" width="33%" alt="Quake III : Arena"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake_iii_team_arena.jpg" width="33%" alt="Quake III : Team Arena"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_urt.jpg" width="33%" alt="Urban Terror">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake2.jpg" width="33%" alt="Quake II"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_the_dark_mod.jpg" width="33%" alt="The Dark Mod"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake1.jpg" width="33%" alt="Quake I">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_return_to_castle_wolfenstein.jpg" width="33%" alt="Return to Castle Wolfenstein"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_wolfenstein_enemy_territory.jpg" width="33%" alt="Wolfenstein: Enemy Territory"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_realrtcw.jpg" width="33%" alt="RealRTCW">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_openja.jpg" width="33%" alt="STAR WARS™ Jedi Knight - Jedi Academy™"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_openjo.jpg" width="33%" alt="STAR WARS™ Jedi Knight II - Jedi Outcast™"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_openmohaa.jpg" width="33%" alt="Medal of Honor: Allied Assault">

----------------------------------------------------------------------------------

> #### Changes:

[Change logs](doc/CHANGES.md ':include')

----------------------------------------------------------------------------------

> #### Build:
* idTech4A++ using std libc's malloc/free in Mem_Alloc/Mem_Free in idlib/Heap.cpp
* idTech4A++ using pure OpenGL programming render pipeline

#### Engine macros
1. **_MULTITHREAD**: Add multithread support for rendering.
2. **_USING_STB**: Using stb jpeg instead of libjpeg for jpeg image file.
3. **_K_CLANG**: If compiling by clang not GCC.
4. **_MODEL_OBJ**: Add obj static model support.
5. **_MODEL_DAE**: Add dae static model support.
6. **_SHADOW_MAPPING**: Add Shadow mapping support.
7. **_OPENGLES3**: Add OpenGLES3.0 support.
8. **_OPENAL** **_OPENAL_EFX** **_OPENAL_SOFT**: Add OpenAL(soft) and EFX Reverb support.
9. **_NO_LIGHT**: Add no lighting support.
10. **_STENCIL_SHADOW_IMPROVE**: Add stencil shadow improve support(translucent shadow, force combine global shadow and self local shadow).
11. **_SOFT_STENCIL_SHADOW**: soft shadow(OpenGLES3.1+), must defined `_STENCIL_SHADOW_IMPROVE` first.
12. **_MINIZ**: Using miniz instead of zlib, using minizip instead of DOOM3's UnZip.
13. **_USING_STB_OGG**: Using stb_vorbis instead of libogg and libvorbis.
14. **_D3BFG_CULLING**: Add DOOM3-BFG occlusion culling support.
15. **_WCHAR_LANG** **_NEW_FONT_TOOLS**: Add wide-character language font support.
16. **_D3BFG_FONT**: Add DOOM3-BFG new font support.
17. **_GLOBAL_ILLUMINATION**: Add global illumination support.
19. **_POSTPROCESS**: Add retro postprocess rendering support.
19. **_GLSL_PROGRAM**: Add GLSL program on new material stage support.
20. **_IMGUI**: Add imGUI support.
21. **_SND_MP3**: Add mp3 sound file support.
22. **_RAVEN_BSE**: Build BSE as effect system on Quake 4.
23. **_RAVEN_FX**: Build Fx as effect system on Quake 4.
24. **_MODEL_MD5V6**: Add 2002 E3 demo md5mesh/md5anim v6 version animation model converter.
25. **_MODEL_PSK**: Support Unreal engine psk/psa animation/static model.
26. **_MODEL_IQM**: Support iqm animation/static model.
27. **_MODEL_SMD**: Support Source engine smd animation/static model.
28. **_MODEL_GLTF**: Support Khronos gltf/glb animation/static model.
29. **_MODEL_FBX**: Support Autodesk fbx animation/static model.

#### * DOOM 3
1. **_DOOM3**: Build DOOM 3 improve changes.
2. **MOD_BOTS**: Build bot support in multiplayer-game.
3. **_MOD_FULL_BODY_AWARENESS**: Build Full-body-awareness mod.
4. **_MOD_VIEW_BODY**: Build view-body mod.
4. **_MOD_VIEW_LIGHT**: Build player flashlight mod.

#### * Quake 4
##### All new sources files put on `raven` folder.
1. **_RAVEN**: Enable Raven Quake 4 patches in engine and idlib source code.
2. **_QUAKE4**: Enable Raven Quake 4 patches in game source code.
3. **_RAVEN_BSE**: Build Raven Quake 4 BSE as particle system.
4. **_RAVEN_FX**: Build DOOM 3 FX instead of BSE as particle system.
5. **MOD_BOTS**: Build bot support in multiplayer-game.
6. **_MOD_FULL_BODY_AWARENESS**: Build Full-body-awareness mod.
7. **_MOD_VIEW_BODY**: Build view-body mod.

#### * Prey(2006)
##### All new sources files put on `humanhead` folder.
1. **_HUMANHEAD**: Enable Humanhead Prey patches in engine and idlib source code.
2. **_PREY**: Enable Humanhead Prey patches in game source code.
3. **_MOD_FULL_BODY_AWARENESS**: Build Full-body-awareness mod.

#### Android
1. **_OPENSLES**: Add OpenSLES support for sound.

#### Linux
1. REQUIRE ALSA, zlib, X11, EGL, SDL2
2. [./bin/cmake_linux_build_doom3_quak4_prey.sh](bin/cmake_linux_build_doom3_quak4_prey.sh ':include')

#### Windows(MSVC)
1. REQUIRE SDL2, cURL, zlib: ```vcpkg install SDL2 curl```
2. Setup your vcpkg.cmake path
3. [/bin/cmake_msvc_build_doom3_quak4_prey.sh](bin/cmake_msvc_build_doom3_quak4_prey.sh ':include')
4. Copy OpenAL32.dll from vcpkg package path to binary path: ```vcpkg install OpenAL-Soft```

----------------------------------------------------------------------------------

> #### Testing version(Non-release. Automatic CI building By Github actions when pushing commits)
* idTech4A++ for Android arm64: [Android testing](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/android_testing).
* idTech4A++ for Android arm32: [Android testing](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/android_testing_armv7).
* DOOM 3/Quake 4/Prey(2006) for Windows x64: [Windows x64 testing](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/windows_x64_testing).

----------------------------------------------------------------------------------

> #### Build DOOM3/Quake4/Prey(2006) mod for Android idTech4A++  

* [DOOM 3 & RoE SDK](https://github.com/glKarin/idtech4amm_doom3_sdk)  
* [Quake 4 SDK](https://github.com/glKarin/idtech4amm_quake4_sdk)  
* [Prey(2006) SDK](https://github.com/glKarin/idtech4amm_prey_sdk) 

----------------------------------------------------------------------------------

> #### Run idTech4A++ on other Android application with Android intent
1. Setup game type with `game` key: also see Q3E/com.n0n3m4.q3e.Q3EGameConstants.java GAME_XXX constants. Valid value: `doom3` `quake4` `prey2006` `quake2` `quake3` `rtcw` `tdm` `quake1` `doom3bfg` `gzdoom` `etw` `realrtcw` `fteqw` `openja` `openjo` `samtfe` `samtse` `urt` `openmohaa` `skindeep` `source`
2. Setup game command arguments with `command` key. Starts with `game.arm`

##### e.g. Run DOOM 3 with custom mod game dll
```
startActivity(new Intent().setComponent(new ComponentName("com.karin.idTech4Amm", "com.n0n3m4.q3e.Q3EMain"))
		.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK)
		// 1. Setup game type
		.putExtra("game", "doom3")
		// 2. Setup game command
		.putExtra("command", "game.arm +set fs_game moddir + harm_fs_gameLibPath /data/app/~~tBiIEpYUhA3P4wkarcd1AA==/com.author.package-p0-kOTTN2iU3ZewCcNXsrA==/lib/arm64/libgame.so")
		);
finish();
```

----------------------------------------------------------------------------------

> #### Bot support on DOOM3/Quake4
1. Extract `doom3/d3_sabot_a7.pk4`(DOOM 3) or `q4base/q4_sabot_a9.pk4`(Quake 4) file in apk to game data folder, it includes some defs, scripts and MP game map AAS file(version < 66).
2. Set cvar `harm_g_autoGenAASFileInMPGame` to 1 for generating a bad AAS file when loading map in Multiplayer-Game and not valid AAS file in current map, you can also put your MP map's AAS file to `maps/mp` folder(botaa48 on DOOM 3, botaa32 on Quake 4).
3. Set `harm_si_autoFillBots` to -1 for automatic fill bots when start MP game.
4. Set `harm_g_botEnableBuiltinAssets` to 1 for enable built-in bot assets if external assets missing(version >= 66), so only need MP game map aas files.

##### cvars:
* harm_si_botLevel: Setup bot level.
* harm_si_botWeapons: Setup bot default weapons.
* harm_si_botAmmo: Setup bot weapons ammo clip.
* harm_si_useCombatBboxInMPGame: Players force use combat bbox in multiplayer game(DOOM 3 only).

##### command:
* addBot: adds a new bot
* removeBot: removes bot specified by id (1,31)
* addBots: add multiplayer bots batch
* removeBots: disconnect multi bots by client ID
* fillBots: fill bots to maximum of server
* appendBots: append more bots(over maximum of server)
* cleanBots: disconnect all bots
* truncBots: disconnect last bots
* botLevel: setup all bot level
* botWeapons: setup all bot initial weapons
* botAmmo: setup all bot initial weapons ammo clip

----------------------------------------------------------------------------------

> #### Full-body-awareness on DOOM3/Quake4/Prey(2006)
1. Make full-body-awareness mode(view player model like first-person view)
[DOOM 3 example](doom3/base/full_body_awareness.cfg ':include')  
[Quake 4 example](doom3/q4base/full_body_awareness.cfg ':include')  
[Prey(2006) example](doom3/preybase/full_body_awareness.cfg ':include')  

2. Make third-person mode and use crosshair(different from `pm_thirdPerson`)
[DOOM 3 example](doom3/base/full_body_awareness_third_persion.cfg ':include')  
[Quake 4 example](doom3/q4base/full_body_awareness_third_persion.cfg ':include')  
[Prey(2006) example](doom3/preybase/full_body_awareness_third_persion.cfg ':include') 

----------------------------------------------------------------------------------

> #### Player view flashlight on DOOM3
[Point flashlight example](doom3/base/show_view_point_flashlight.cfg ':include')  
[Spot flashlight example](doom3/base/show_view_spot_flashlight.cfg ':include')  

----------------------------------------------------------------------------------

> #### Player body view on DOOM3/Quake4
[DOOM 3 example](doom3/base/def/player_viewbody.cfg ':include')  
[Quake 4 example](doom3/q4base/def/player_viewbody.cfg ':include')  

----------------------------------------------------------------------------------

> #### on F-Droid

| Feature                             | Github | F-Droid |
|:------------------------------------|:------:|:-------:|
| Android min version(for ffmpeg) |   4.4  |   7.0   |
| Khronos Vulkan validation layer     |   Yes  |    No   |

---------------------------------------------------------------------------------- 

> #### Compare with other OpenGLES rendering version of DOOM3  

| Feature | idTech4A++ | Other |
|:-----|:-----:|:-----:|
| Multi-threading | Support<br/>(using `multithread` command to switch if enabled in gaming) | d3es-multithread support<br/>(and support switch in gaming) |
| New stage shader<br/>(heatHaze, heatHazeWithMask, heatHazeWithMaskAndVertex, colorProcess, enviroSuit(D3XP)) | Yes | - |
| No lighting | Yes<br/>(And support switch in gaming by set harm_r_lightingModel to 0) | Yes |
| TexGen | Yes | - |
| OpenGL ES version | 2.0 and 3.0+<br/>(point light shadow mapping shader use cubemap on OpenGLES2.0, use texture array on OpenGLES3.0+) | 2.0(3.0+ compat) |
| Debug render tools | Yes<br/>(need to set harm_r_renderToolsMultithread to 1 if with multi-threading) | - |

----------------------------------------------------------------------------------

> #### Branch:

##### master:
* /idTech4Amm: launcher source
* /Q3E: frontend source
* /doom3: DOOM 3/Quake 4/Prey(2006) source

##### free:
* For F-Droid pure free version.

##### package:
* /screenshot: screenshot pictures
* /source: Reference source
* /pak: Game resource

##### n0n3m4_original_old_version:
* Original old `n0n3m4` version source.

----------------------------------------------------------------------------------

> #### Extras download:

* [Baidu网盘: https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w 提取码: `pyyj`](https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w?pwd=pyyj)
* [Baidu贴吧: BEYONDK2000](https://tieba.baidu.com/p/6825594793)
* [F-Droid(different signature)](https://f-droid.org/packages/com.karin.idTech4Amm/)

----------------------------------------------------------------------------------
