# idTech4A++ (Harmattan Edition)  <img align="right" width="128" height="128" src="https://github.com//glKarin/com.n0n3m4.diii4a/raw/master/idTech4Amm/src/main/res/drawable/icon.png" alt="idTech4A++" />  
[![Android Build Actions Status](https://github.com/glKarin/com.n0n3m4.diii4a/actions/workflows/android.yml/badge.svg)](https://github.com/glKarin/com.n0n3m4.diii4a/actions/workflows/android.yml) [![Windows/Linux Build Actions Status](https://github.com/glKarin/com.n0n3m4.diii4a/actions/workflows/win_linux.yml/badge.svg)](https://github.com/glKarin/com.n0n3m4.diii4a/actions/workflows/win_linux.yml)  
[![Discord chat](https://img.shields.io/discord/1398154850239254568.svg?logo=discord&label=Discord%20chat)](https://discord.gg/KFshBra4kh)  
[![Latest Release](https://img.shields.io/github/downloads/glKarin/com.n0n3m4.diii4a/latest/total)](https://github.com/glKarin/com.n0n3m4.diii4a/releases)  
[![Download Android testing](https://img.shields.io/github/downloads/glKarin/com.n0n3m4.diii4a/android_testing/total?label=downloads%40Android%20testing
)](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/android_testing) [![Download Windows/Linux testing](https://img.shields.io/github/downloads/glKarin/com.n0n3m4.diii4a/win_linux_testing/total?label=downloads%40Windows%2FLinux%20testing
)](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/win_linux_testing)  
**idTech** engine games **For** **A**ndroid. An **idTech** games runtime libraries collection on Android  
#### DOOM III/Quake 4/Prey(2006) OpenGLES on Android/Windows/Linux  
##### DOOM 3 BFG/The Dark Mod/Quake 1 2 3/RTCW/GZDOOM/ETW/RealRTCW/FTEQW/STAR WARS™ Jedi Knight/Serious Sam Classic/Urban Terror/OpenMOHAA on Android  
#### 毁灭战士3/雷神之锤4/掠食(2006) 安卓/Windows/Linux OpenGLES移植版  
##### 毁灭战士3 BFG/The Dark Mod/雷神之锤1 2 3/重返德军总部/GZDOOM/深入敌后: 德军总部/真·重返德军总部/FTEQW/星球大战:绝地武士/英雄萨姆 安卓移植版  
##### Original named DIII4A++, based on com.n0n3m4.diii4a's OpenGLES version.
**Latest version:**
1.1.0harmattan70(lindaiyu)  
**Latest update:**
2025-12-21  
**Arch:**
arm64 armv7-a  
**Platform:**
Android 4.4+  
**License:**
GPLv3  

----------------------------------------------------------------------------------

> ### idTech4's feature 
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

> ### Compare with other OpenGLES rendering version of DOOM3  

| Feature | idTech4A++ | Other |
|:-----|:-----:|:-----:|
| Multi-threading | Support<br/>(but can't switch in gaming) | d3es-multithread support<br/>(and support switch in gaming) |
| New stage shader<br/>(heatHaze, heatHazeWithMask, heatHazeWithMaskAndVertex, colorProcess, enviroSuit(D3XP)) | Yes | - |
| No lighting | Yes<br/>(And support switch in gaming by set harm_r_lightingModel to 0) | Yes |
| TexGen | Yes | - |
| OpenGL ES version | 2.0 and 3.0+<br/>(point light shadow mapping shader use cubemap on OpenGLES2.0, use texture array on OpenGLES3.0+) | 2.0(3.0+ compat) |
| Debug render tools | Yes<br/>(need to set harm_r_renderToolsMultithread to 1 if with multi-threading) | - |

----------------------------------------------------------------------------------

> ### Support games

| Game | Engine | Version | OpenGL ES/Vulkan version | Mods |
|:-----|:-----:|:-----:|:-----:|:-----:|
| DOOM III | n0n3m4's dante | - | 2.0/3.0 | [Resurrection of Evil]()<br/>[The Lost Mission](https://www.moddb.com/mods/the-lost-mission)<br/>[Classic DOOM3](https://www.moddb.com/mods/classic-doom-3)<br/>[Rivensin](https://www.moddb.com/mods/ruiner)<br/>[HardCorps](https://www.moddb.com/mods/hardcorps)<br/>[Overthinked Doom^3](https://www.moddb.com/mods/overthinked-doom3)<br/>[Sabot(a7x)](https://www.moddb.com/games/doom-3-resurrection-of-evil/downloads/sabot-alpha-7x)<br/>[HeXen:Edge of Chaos](https://www.moddb.com/mods/hexen-edge-of-chaos)<br/>[Fragging Free](https://www.moddb.com/mods/fragging-free)<br/>[LibreCoop](https://www.moddb.com/mods/librecoop-dhewm3-coop)<br/>[LibreCoop D3XP](https://www.moddb.com/mods/librecoop-dhewm3-coop)<br/>[Perfected Doom 3](https://www.moddb.com/mods/perfected-doom-3-version-500)<br/>[Perfected Doom 3:RoE](https://www.moddb.com/mods/perfected-doom-3-version-500)<br/>[Doom 3: Phobos](https://www.moddb.com/mods/phobos)([Dhewm3 compatibility patch](https://www.moddb.com/games/doom-iii/addons/doom-3-phobos-dhewm3-compatibility-patch)) |
| Quake IV | n0n3m4's dante | - | 2.0/3.0 | [Hardqore](https://www.moddb.com/mods/quake-4-hardqore) |
| Prey(2006) | n0n3m4's dante | - | 2.0/3.0 |  |
| DOOM 3 BFG(Classic DOOM 1&2) | [RBDOOM-3-BFG](https://github.com/RobertBeckebans/RBDOOM-3-BFG) | 1.4.0<br/>(The last OpenGL renderer version) | 3.0/Vulkan1.1 |  |
| The Dark Mod | [Dark Mod](https://www.thedarkmod.com) | 2.13 | 3.2<br/>(require geometry shader support) |  |
| Return to Castle Wolfenstein | [iortcw](https://github.com/iortcw/iortcw) | 1.51d | 1.1 |  |
| Quake III Arena | [ioquake3](https://github.com/ioquake/ioq3) | 1.36 | 1.1 | Quake III Team Arena |
| Quake II | [Yamagi Quake II](https://github.com/yquake2/yquake2) | 8.60 | 1.1/3.2/Vulkan | Capture The Flag<br/>Ground Zero<br/>The Reckoning<br/>Team Evolves Zaero<br/>3rd Zigock Bot II |
| Quake I | [Darkplaces](https://github.com/DarkPlacesEngine/darkplaces) | - | 2.0 |  |
| GZDOOM | [GZDOOM](https://github.com/ZDoom/gzdoom) 64bits | 4.14.2 | 2.0/3.2/Vulkan |  |
| Wolfenstein: Enemy Territory | [ET: Legacy](https://www.etlegacy.com) Omni-Bot support | 2.83.2 | 1.1 |  |
| RealRTCW | [RealRTCW](https://github.com/wolfetplayer/RealRTCW) | 5.3 | 1.1 |  |
| FTEQW | [FTEQW](https://www.fteqw.org) | 1.05 | 3.2/Vulkan |  |
| STAR WARS™ Jedi Knight - Jedi Academy™ | [OpenJK](https://github.com/JACoders/OpenJK) | 1.0.1.1 | 1.1 |  |
| STAR WARS™ Jedi Knight II - Jedi Outcast™ | [OpenJK](https://github.com/JACoders/OpenJK) | 1.0.1.1 | 1.1 |  |
| Serious Sam Classic : The First Encounter | [SamTFE](https://github.com/tx00100xt/SeriousSamClassic) | 1.10.7 | 1.1 |  |
| Serious Sam Classic : The Second Encounter | [SamTSE](https://github.com/tx00100xt/SeriousSamClassic) | 1.10.7 | 1.1 |  |
| Urban Terror | [Q3-UT4](https://www.urbanterror.info) | 4.3.4 | 1.1 |  |
| Medal of Honor: Allied Assault | [OpenMOHAA](https://github.com/openmoh/openmohaa) | 0.82.1 | 1.1 |  |

----------------------------------------------------------------------------------

> ### on F-Droid

[<img src="https://fdroid.gitlab.io/artwork/badge/get-it-on.png"
     alt="Get it on F-Droid"
     height="80">](https://f-droid.org/packages/com.karin.idTech4Amm/)

Or download the latest APK from the [Releases Section](https://github.com/glKarin/com.n0n3m4.diii4a/releases/latest).
Tag with `-free` only for F-Droid update.

| Feature                             | Github | F-Droid |
|:------------------------------------|:------:|:-------:|
| Android min version(because ffmpeg) |   4.4  |   7.0   |
| Khronos Vulkan validation layer     |   Yes  |    No   |

----------------------------------------------------------------------------------

> ### Build DOOM3/Quake4/Prey(2006) mod for Android idTech4A++  

* [DOOM 3 & RoE SDK](https://github.com/glKarin/idtech4amm_doom3_sdk)  
* [Quake 4 SDK](https://github.com/glKarin/idtech4amm_quake4_sdk)  
* [Prey(2006) SDK](https://github.com/glKarin/idtech4amm_prey_sdk) 

----------------------------------------------------------------------------------

> ### Testing version(Non-release. Automatic CI building By Github actions when pushing commits)
* idTech4A++ for Android: [Android testing](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/android_testing).
* DOOM 3/Quake 4/Prey(2006) for Windows x64: [Windows x64 testing](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/windows_x64_testing).

----------------------------------------------------------------------------------

> ### Update 1.1.0harmattan70 (2025-12-21)

* Fix font offset in GUIs on Quake 4.
* Fix a sound distance volume bug on Quake 4.
* Support smooth joystick on DOOM 3 mod `Hardscorps` and Quake 4 mod `Hardqore`.
* Add `Medal of Honor: Allied Assault`(ver 0.82.1) support, game standalone directory named `openmohaa`, game data directory named `main`. More view in [OpenMOHAA](https://github.com/openmoh/openmohaa).
* Update RealRTCW version to 5.3, version 5.2 will be removed on next release.
* Update ioquake3(Quake 3), Darkplace(Quake 1), OpenJK, yquake2(Quake 2), and add 3rd Zigock Bot II mod support on Quake 2.
* On-screen button keymap and layout configures are standalone in each game.
* Add Unreal engine psk/psa, iqm, Source engine smd, GLTF/GLB, Autodesk fbx animation/static model support, add md5mesh static model support on DOOM3/Quake 4/Prey.
* Add float console support on DOOM3/Quake 4/Prey.
* Fix game start on arm32 device.
* Warning: FTEQW is removed in this release.

----------------------------------------------------------------------------------
  
##### Games of `Standalone game directory` and folder name:
* **DOOM III**: doom3/
* **Quake 4**: quake4/
* **Prey(2006)**: prey/
* **Quake I**: quake1/
* **Quake II**: quake2/
* **Quake III**: quake3/
* **Return to Castle Wolfenstein**: rtcw/
* **DOOM 3 BFG**: doom3bfg/
* **Wolfenstein - Enemy Territory**: etw/
* **RealRTCW**: realrtcw/
* **STAR WARS™ Jedi Knight - Jedi Academy™**: openja/
* **STAR WARS™ Jedi Knight II - Jedi Outcast™**: openjo/
* **Urban Terror**: urt/
* **Medal of Honor: Allied Assault**: openmohaa/
  
##### Games of always force `Standalone game directory`:
* **The Dark Mod**: darkmod/
* **GZDOOM**: gzdoom/
* **FTEQW**: fteqw/
* **Serious Sam Classic - The First Encounter**: serioussamtfe/
* **Serious Sam Classic - The Second Encounter**: serioussamtse/

----------------------------------------------------------------------------------

> ### idTech4's new Cvar
##### Flag
* ARCHIVE = save to/load from config file
* FIXED = can't change in game
* READONLY = readonly, always can't change
* INIT = only setup on boot command
* ISSUE = maybe has bugs
* DEBUG = only for developer debug and test
* DISABLE = only for disable something and keep original source code

| CVar | Type | Default | Description | Flag | Range | Scope | Remark | Platform |
|:---|:---:|:--:|:---|:---:|:---:|:---|:---|:---:|
| harm_r_openglVersion | String | GLES3.0 | OpenGL version | INIT | GLES2, GLES3.0, OpenGL_core, OpenGL_compatibility | Engine/Renderer | OpenGL_core, OpenGL_compatibility will use OpenGL desktop version, setup with launcher on Android | Windows, Linux |
| r_multithread | Bool | 1 | Multithread backend | INIT |  | Engine/Renderer | setup with launcher on Android | Windows, Linux |
| harm_r_clearVertexBuffer | Integer | 2 | Clear vertex buffer on every frame | ARCHIVE, FIXED | 0, 1, 2 | Engine/Renderer | 0 = not clear(original);<br/> 1 = only free VBO memory;<br/> 2 = free VBO memory and delete VBO handle | All |
| harm_r_maxAllocStackMemory | Integer | 262144 | Control allocate temporary memory when load model data | ARCHIVE |  | Engine/Renderer | For load large model, because stack memory is limited on OS.<br/> 0 = Always heap;<br/> Negative = Always stack;<br/> Positive = Max stack memory limit(If less than this `byte` value, call `alloca` in stack memory, else call `malloc`/`calloc` in heap memory) | All |
| harm_r_shadowCarmackInverse | Bool | 0 | Stencil shadow using Carmack-Inverse | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_globalIllumination | Bool | 0 | Render global illumination before draw interactions | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_globalIlluminationBrightness | Float | 0.3 | Global illumination brightness | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_lightingModel | Integer | 1 | Lighting model when draw interactions | ARCHIVE | 0, 1, 2, 3 | Engine/Renderer | 1 = Phong(default);<br/> 2 = Blinn-Phong;<br/> 3 = PBR;<br/> 0 = Ambient(no lighting) | All |
| harm_r_specularExponent | Float | 3.0 | Specular exponent in Phong interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_specularExponentBlinnPhong | Float | 12.0 | Specular exponent in Blinn-Phong interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_specularExponentPBR | Float | 5.0 | Specular exponent in PBR interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_PBRNormalCorrection | Float | 0.25 | Vertex normal correction(surface smoothness) in PBR interaction lighting model | ARCHIVE | [0.0 - 1.0] | Engine/Renderer | 1 = pure using bump texture(lower smoothness); 0 = pure using vertex normal(high smoothness); 0.0 - 1.0 = bump texture * harm_r_PBRNormalCorrection + vertex normal * (1 - harm_r_PBRNormalCorrection) | All |
| harm_r_PBRRoughnessCorrection | Float | 0.55 | max roughness for old specular texture | ARCHIVE | &gt;= 0.0 | Engine/Renderer | 0 = disable; else = roughness = harm_r_PBRRoughnessCorrection - texture(specularTexture, st).r | All |
| harm_r_PBRMetallicCorrection | Float | 0 | min metallic for old specular texture | ARCHIVE | &gt;= 0.0 | Engine/Renderer | 0 = disable; else = metallic = texture(specularTexture, st).r + harm_r_PBRMetallicCorrection | All |
| harm_r_PBRRMAOSpecularMap | Bool | 0 | pecular map is standard PBR RAMO texture or old non-PBR texture | ARCHIVE |  | Engine/Renderer |  | All |
| r_maxFps | Integer | 0 | Limit maximum FPS. | ARCHIVE | &gt;= 0 | Engine/Renderer | 0 = unlimited | All |
| r_screenshotFormat | Integer | 0 | Screenshot format | ARCHIVE | 0, 1, 2, 3, 4 | Engine/Renderer | 0 = TGA (default),<br/> 1 = BMP,<br/> 2 = PNG,<br/> 3 = JPG,<br/> 4 = DDS | All |
| r_screenshotJpgQuality | Integer | 75 | Screenshot quality for JPG images | ARCHIVE | [0 - 100] | Engine/Renderer |  | All |
| r_screenshotPngCompression | Integer | 3 | Compression level when using PNG screenshots | ARCHIVE | [0 - 9] | Engine/Renderer |  | All |
| r_useShadowMapping | Bool | 0 | use shadow mapping instead of stencil shadows | ARCHIVE | All | Engine/Renderer |  | All |
| harm_r_shadowMapAlpha | Float | 1.0 | shadow's alpha in shadow mapping | ARCHIVE | [0.0 - 1.0] | Engine/Renderer |  | All |
| harm_r_shadowMapJitterScale | Float | 2.5 | scale factor for jitter offset | ARCHIVE |  | Engine/Renderer |  | All |
| r_forceShadowMapsOnAlphaTestedSurfaces | Bool | DOOM3 is 1, Quake4/Prey is 0 | render performed material shadow in shadow mapping | ARCHIVE, ISSUE |  | Engine/Renderer | 0 = same shadowing as with stencil shadows,<br/> 1 = ignore noshadows for alpha tested materials | All |
| harm_r_shadowMapCombine | Bool | 1 | combine local and global shadow mapping | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_shadowMapLightType | Integer | 0 | debug light type mask in shadow mapping | DEBUG | 0, 1, 2, 3, 4, 5, 6, 7 | Engine/Renderer | 1 = parallel;<br/> 2 = point;<br/> 4 = spot | All |
| harm_r_shadowMapDepthBuffer | Integer | 0 | render depth to color or depth texture in OpenGLES2.0 | INIT, DEBUG | 0, 1, 2, 3, 4 | Engine/Renderer | 0 = Auto;<br/> 1 = depth texture;<br/> 2 = color texture's red;<br/> 3 = color texture's rgba | All |
| harm_r_shadowMapNonParallelLightUltra | Bool | 0 | non parallel light allow ultra quality shadow map texture |  |  | Engine/Renderer | max texture size is 2048x2048 | All |
| harm_r_shadowMapLod | Integer | -1 | force using shadow map LOD |  | -1, 0, 1, 2, 3, 4 | Engine/Renderer | -1 = auto | All |
| r_shadowMapFrustumFOV | Float | 90 | oversize FOV for point light side matching |  |  | Engine/Renderer |  | All |
| harm_r_stencilShadowTranslucent | Bool | 0 | enable translucent shadow in stencil shadow | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_stencilShadowAlpha | Float | 1.0 | translucent shadow's alpha in stencil shadow | ARCHIVE | [0.0 - 1.0] | Engine/Renderer |  | All |
| harm_r_stencilShadowCombine | Bool | 0 | combine local and global stencil shadow | ARCHIVE, ISSUE |  | Engine/Renderer |  | All |
| harm_r_stencilShadowSoft | Bool | 0 | enable soft stencil shadow | ARCHIVE | only OpenGLES3.1+ | Engine/Renderer |  | All |
| harm_r_stencilShadowSoftBias | Float | -1 | soft stencil shadow sampler BIAS | ARCHIVE |  | Engine/Renderer | -1 to automatic; 0 = disable; positive = value | All |
| harm_r_stencilShadowSoftCopyStencilBuffer | Bool | 0 | copy stencil buffer directly for soft stencil shadow | ARCHIVE | 0 = copy depth buffer and bind and renderer stencil buffer to texture directly<br/>1 = copy stencil buffer to texture directly | Engine/Renderer |  | All |
| r_renderMode | Integer | 0 | retro postprocess render | ARCHIVE | 0 = Doom<br/>1 = CGA<br/>2 = CGA Highres<br/>3 = Commodore 64<br/>4 = Commodore 64 Highres<br/>5 = Amstrad CPC 6128<br/>6 = Amstrad CPC 6128 Highres<br/>7 = Sega Genesis<br/>8 = Sega Genesis Highres<br/>9 = Sony PSX | Engine/Renderer |  | All |
| harm_r_autoAspectRatio | Integer | 1 | automatic setup aspect ratio of view | ARCHIVE | 0, 1, 2 | Engine/Renderer | 0 = manual<br/>1 = force setup r_aspectRatio to -1<br/>2 = automatic setup r_aspectRatio to 0,1,2 by screen size | Android |
| harm_r_renderToolsMultithread | Bool | 1 | Enable render tools debug with GLES in multi-threading | ARCHIVE |  | Engine/Renderer |  | All |
| r_useETC1 | Bool | 0 | use ETC1 compression | INIT |  | Engine/Renderer |  | All |
| r_useETC1cache | Bool | 0 | use ETC1 compression | INIT |  | Engine/Renderer |  | All |
| r_useDXT | Bool | 0 | use DXT compression if possible | INIT |  | Engine/Renderer |  | All |
| r_useETC2 | Bool | 0 | use ETC2 compression instead of RGBA4444 | INIT |  | Engine/Renderer | Only for OpenGLES3.0+ | All |
| r_noLight | Bool | 0 | lighting disable hack | INIT |  | Engine/Renderer | 1 = disable lighting(not allow switch, must setup on command line) | All |
| r_showStencil | Bool | 0 | display the contents of the stencil index buffer |  |  | Engine/Renderer |  | All |
| harm_r_useGLSLShaderBinaryCache | Integer | 0 | Use GLSL shader compiled binary cache | INIT | 0, 1, 2 | Engine/Renderer | 0 = Disable<br/>1 = Enable and check<br/>2 = Enable and uncheck | All |
| harm_r_useHighPrecision | Integer | Android = 0; Other = 1 | Use high precision float on GLSL shade | INIT | 0, 1, 2 | Engine/Renderer | 0 = use default precision(interaction/depth shaders use high precision, otherwise use medium precision)<br/>1 = all shaders use high precision as default precision exclude special variables<br/>2 = all shaders use high precision as default precision and special variables also use high precision | All |
| harm_r_occlusionCulling | Bool | 0 | enable DOOM3-BFG occlusion culling | ARCHIVE |  | Engine/Renderer |  | All |
| harm_fs_gameLibPath | String |  | Setup game dynamic library | ARCHIVE |  | Engine/Framework |  | Android |
| harm_fs_gameLibDir | String |  | Setup game dynamic library directory path | ARCHIVE |  | Engine/Framework |  | Android |
| harm_com_consoleHistory | Integer | 2 | Save/load console history | ARCHIVE | 0, 1, 2 | Engine/Framework | 0 = disable;<br/> 1 = loading in engine initialization, and saving in engine shutdown;<br/> 2 = loading in engine initialization, and saving in every executing | All |
| harm_con_float | Bool | 0 | Enable float console | ARCHIVE |  | Engine/Framework |  | All |
| harm_con_alwaysShow | Bool | 0 | Always show console | ARCHIVE |  | Engine/Framework |  | All |
| harm_con_noBackground | Bool | 0 | Don't draw console background | ARCHIVE |  | Engine/Framework |  | All |
| harm_con_floatGeometry | String | 100 50 300 200 | Float console geometry, format is "<left> <top> <width> <height>"<br/>Holding left mouse or CTRL and moving mouse to move float console position<br/>Holding left mouse or CTRL and swiping mouse wheel to resize float console. | ARCHIVE |  | Engine/Framework |  | All |
| harm_con_floatZoomStep | Integer | 10 | Zoom step of float console when holding left mouse or CTRL and swiping mouse wheel for resize float console | ARCHIVE |  | Engine/Framework |  | All |

| com_disableAutoSaves | Bool | 0 | Don't create Autosaves when entering a new map | ARCHIVE |  | Engine/Framework |  | All |
| harm_fs_basepath_extras | String |  | Extras search paths last(split by ',') | INIT |  | Engine/Framework |  | All |
| harm_fs_addon_extras | String |  | Extras search addon files directory path last(split by ',') | INIT |  | Engine/Framework |  | All |
| harm_fs_game_base_extras | String |  | Extras search game mod last(split by ',') | INIT |  | Engine/Framework |  | All |
| r_scaleMenusTo43 | Integer | 0 | Scale menus, fullscreen videos and PDA to 4:3 aspect ratio | ARCHIVE | 0, 1, -1 | Engine/GUI | 0 = disable;<br/> 1 = only scale menu type GUI as 4:3 aspect ratio;<br/> -1 = scale all GUI as 4:3 aspect ratio | All |
| harm_r_shaderProgramDir | String |  | Setup external OpenGLES2 GLSL shader program directory path | ARCHIVE |  | Engine/Renderer | empty is glslprogs(default) | All |
| harm_r_shaderProgramES3Dir | String |  | Setup external OpenGLES3 GLSL shader program directory path | ARCHIVE |  | Engine/Renderer | empty is glsl3progs(default) | All |
| harm_r_debugOpenGL | Bool | 0 | debug OpenGL | ARCHIVE | only OpenGLES3.1+ | Engine/Renderer |  | All |
| harm_gui_wideCharLang | Bool | 0 | enable wide character language support | ARCHIVE |  | Engine/GUI |  | All |
| harm_gui_useD3BFGFont | String | 0 | use DOOM3-BFG fonts instead of old fonts | INIT | 0, "", 1, &lt;DOOM3-BFG font name&gt; | Engine/GUI | 0 or "" = disable;<br/><br/> 1 = make DOOM3 old fonts mapping to DOOM3-BFG new fonts automatic. e.g. <br/> - In DOOM 3: <br/> * 'fonts/fontImage_xx.dat' -&gt; 'newfonts/Chainlink_Semi_Bold/48.dat'<br/> * 'fonts/an/fontImage_xx.dat' -&gt; 'newfonts/Arial_Narrow/48.dat'<br/> * 'fonts/arial/fontImage_xx.dat' -&gt; 'newfonts/Arial_Narrow/48.dat'<br/> * 'fonts/bank/fontImage_xx.dat' -&gt; 'newfonts/BankGothic_Md_BT/48.dat'<br/> * 'fonts/micro/fontImage_xx.dat' -&gt; 'newfonts/microgrammadbolext/48.dat'<br/> - In Quake 4(`r_strogg` and `strogg` fonts always disable): <br/> *  'fonts/chain_xx.dat' -&gt; 'newfonts/Chainlink_Semi_Bold/48.dat'<br/> * 'fonts/lowpixel_xx.dat' -&gt; 'newfonts/microgrammadbolext/48.dat'<br/> * 'fonts/marine_xx.dat' -&gt; 'newfonts/Arial_Narrow/48.dat'<br/> * 'fonts/profont_xx.dat' -&gt; 'newfonts/BankGothic_Md_BT/48.dat'<br/> - In Prey(`alien` font always disable): <br/> * 'fonts/fontImage_xx.dat' -&gt; 'newfonts/Chainlink_Semi_Bold/48.dat'<br/> * 'fonts/menu/fontImage_xx.dat' -&gt; 'newfonts/Arial_Narrow/48.dat'<br/><br/> &lt;DOOM3-BFG font name&gt; = use a DOOM3-BFG new font by name override all DOOM 3/Quake 4/Prey old fonts. e.g.<br/> * Chainlink_Semi_Bold<br/> * Arial_Narrow<br/> * BankGothic_Md_BT<br/> * microgrammadbolext<br/> * DFPHeiseiGothicW7<br/> * Sarasori_Rg | All |
| s_driver | String | AudioTrack | sound driver | ARCHIVE | AudioTrack, OpenSLES | Engine/Sound | sound if without OpenAL on Android | Android |
| harm_s_OpenSLESBufferCount | Integer | 3 | Audio buffer count for OpenSLES | ARCHIVE | &gt;= 3 | Engine/Sound | min is 3, only for if without OpenAL and use OpenSLES on Android | Android |
| harm_s_useAmplitudeDataOpenAL | Bool | 0 | Use amplitude data on OpenAL | DISABLE, ISSUE |  | Engine/Sound | It cause large shake | All |
| harm_in_smoothJoystick | Bool | 0 | Enable smooth joystick |  |  | Engine/Input | Automatic setup initial value by Android layer | Android |
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset, format is "&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;" | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | Head | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_showViewBody | Bool | 0 | Show view body | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_showViewLight | Bool | 0 | show player view flashlight | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_viewLightShader | String | lights/flashlight5 | player view flashlight material texture/entityDef name | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_viewLightRadius | Vector3 String | 1280 640 640 | player view flashlight radius, format is "&lt;light_target&gt; &lt;light_right&gt; &lt;light_up&gt;" | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_viewLightOffset | Vector3 String | 0 0 0 | player view flashlight origin offset, format is "&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;" | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_viewLightType | Integer | 0 | player view flashlight type | ARCHIVE | 0=spot light; 1=point light | Game/DOOM3 |  | All |
| harm_ui_viewLightOnWeapon | Bool | 0 | player view flashlight follow weapon position | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_g_autoGenAASFileInMPGame | Bool | 1 | For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_si_autoFillBots | Bool | 0 | Automatic fill bots after map loaded in multiplayer game | ARCHIVE | &gt;=0 | Game/DOOM3 | 0 = disable; other number = bot num | All |
| harm_si_botLevel | Integer | 0 | Bot level | ARCHIVE | [0 - 8] | Game/DOOM3 | 0 = auto; 1 - 8 = difficult level | All |
| harm_si_botWeapons | String | 0 | Bot initial weapons when spawn, separate by comma(,) | ARCHIVE | 1=pistol; 2=shotgun; 3=machinegun; 4=chaingun; 5=handgrenade; 6=plasmagun; 7=rocketlauncher; 8=BFG; 10=chainsaw; 0=none; *=all | Game/DOOM3 | Allow weapon index(e.g. 2,3), weapon short name(e.g. shotgun,machinegun), weapon full name(e.g. weapon_shotgun,weapon_machinegun), and allow mix(e.g. shotgun,3,weapon_rocketlauncher) | All |
| harm_si_botAmmo | Integer | 0 | Bot weapons initial ammo clip when spawn, depend on harm_si_botWeapons | ARCHIVE |  | Game/DOOM3 | -1=max ammo, 0=none, &gt;0=ammo clip | All |
| harm_g_botEnableBuiltinAssets | Bool | 0 | Enable built-in bot assets if external assets missing | INIT |  | Game/DOOM3 |  | All |
| harm_si_useCombatBboxInMPGame | Bool | 0 | Players force use combat bbox in multiplayer game | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_doubleJump | Bool | 0 | Enable double-jump | ARCHIVE |  | Game/DOOM3/Rivensin |  | All |
| harm_pm_autoForceThirdPerson | Bool | 1 | Force set third person view after game level load end | ARCHIVE |  | Game/DOOM3/Rivensin |  | All |
| harm_pm_preferCrouchViewHeight | Float | 32 | Set prefer crouch view height in Third-Person | ARCHIVE | &gt;= 0 | Game/DOOM3/Rivensin | suggest 32 - 39, less or equals 0 to disable | All |
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset, format is "&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;" | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | head_channel | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_gui_defaultFont | String | chain | Default font name | ARCHIVE | chain, lowpixel, marine, profont, r_strogg, strogg | Engine/Quake4/GUI | It will be available in next running | All |
| harm_ui_showViewBody | Bool | 0 | Show view body | ARCHIVE |  | Game/Quake4 |  | All |
| harm_g_autoGenAASFileInMPGame | Bool | 1 | For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic | ARCHIVE |  | Game/Quake4 |  | All |
| harm_si_autoFillBots | Bool | 0 | Automatic fill bots after map loaded in multiplayer game | ARCHIVE | &gt;=0 | Game/Quake4 | 0 = disable; other number = bot num | All |
| harm_si_botLevel | Integer | 0 | Bot level | ARCHIVE | [0 - 8] | Game/Quake4 | 0 = auto; 1 - 8 = difficult level | All |
| harm_si_botWeapons | String | 0 | Bot initial weapons when spawn, separate by comma(,) | ARCHIVE | 1=machinegun; 2=shotgun; 3=hyperblaster; 4=grenadelauncher; 5=nailgun; 6=rocketlauncher; 7=railgun; 8=lightninggun; 9=dmg; 10=napalmgun; 0=none; *=all | Game/Quake4 | Allow weapon index(e.g. 2,3), weapon short name(e.g. shotgun,machinegun), weapon full name(e.g. weapon_machinegun,weapon_shotgun), and allow mix(e.g. machinegun,3,weapon_rocketlauncher) | All |
| harm_si_botAmmo | Integer | 0 | Bot weapons initial ammo clip when spawn, depend on harm_si_botWeapons | ARCHIVE |  | Game/Quake4 | -1=max ammo, 0=none, &gt;0=ammo clip | All |
| harm_g_botEnableBuiltinAssets | Bool | 0 | Enable built-in bot assets if external assets missing | INIT |  | Game/Quake4 |  | All |
| harm_g_mutePlayerFootStep | Bool | 0 | mute player's footstep sound | ARCHIVE |  | Game/Quake4 |  | All |
| harm_g_allowFireWhenFocusNPC | Bool | 0 | allow fire when focus NPC | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset, format is "&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;" | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | neck | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_ui_translateAlienFont | String | fonts | Setup font name for automatic translate `alien` font text of GUI | ARCHIVE | fonts, fonts/menu, "" | Engine/Prey/GUI | empty to disable | All |
| harm_ui_translateAlienFontDistance | Float | 200 | Setup max distance of GUI to view origin for enable translate `alien` font text | ARCHIVE |  | Engine/Prey/GUI | 0 = disable;<br/> -1 = always;<br/> positive: distance value | All |
| harm_ui_subtitlesTextScale | Float | 0.32 | Subtitles's text scale | ARCHIVE |  | Engine/Prey/GUI | &lt;= 0 to unset | All |
| harm_r_skipHHBeam | Bool | 0 | Skip beam model render | DEBUG |  | Engine/Prey/Renderer |  | All |

----------------------------------------------------------------------------------

> ### idTech4's new command

| Command | Description | Usage | Scope | Remark | Platform |
|:---|:---|:---|:---|:---|:---:|
| exportGLSLShaderSource | export GLSL shader source to filesystem |  | Engine/Renderer | Only export shaders of using OpenGLES2.0 or OpenGLES3.0 | All |
| printGLSLShaderSource | print internal GLSL shader source |  | Engine/Renderer | Only print shaders of using OpenGLES2.0 or OpenGLES3.0 | All |
| exportDevShaderSource | export internal original C-String GLSL shader source for developer |  | Engine/Renderer | Export all shaders of OpenGLES2.0 and OpenGLES3.0 | All |
| convertARB | convert ARB shader to GLSL shader |  | Engine/Renderer | It has many errors, only port some ARB shader to GLSL shader | All |
| reloadGLSLprograms | reloads GLSL programs |  | Engine/Renderer |  | All |
| cleanExternalGLSLShaderSource | Remove external GLSL shaders directory |  | Engine/Renderer |  | All |
| cleanGLSLShaderBinary | Remove GLSL shader binaries directory |  | Engine/Renderer |  | All |
| pskToMd5mesh | Convert psk to md5mesh |  | Engine/Renderer |  | All |
| psaToMd5anim | Convert psa to md5anim |  | Engine/Renderer |  | All |
| pskPsaToMd5 | Convert psk/psa to md5mesh/md5anim |  | Engine/Renderer |  | All |
| iqmToMd5mesh | Convert iqm to md5mesh |  | Engine/Renderer |  | All |
| iqmToMd5anim | Convert iqm to md5anim |  | Engine/Renderer |  | All |
| iqmToMd5 | Convert iqm to md5mesh/md5anim |  | Engine/Renderer |  | All |
| smdToMd5mesh | Convert smd to md5mesh |  | Engine/Renderer |  | All |
| smdToMd5anim | Convert smd to md5anim |  | Engine/Renderer |  | All |
| smdToMd5 | Convert smd to md5mesh/md5anim |  | Engine/Renderer |  | All |
| fbxToMd5mesh | Convert fbx to md5mesh |  | Engine/Renderer |  | All |
| fbxToMd5anim | Convert fbx to md5anim |  | Engine/Renderer |  | All |
| fbxToMd5 | Convert fbx to md5mesh/md5anim |  | Engine/Renderer |  | All |
| md5meshV6ToV10 | Convert md5mesh v6(2002 E3 demo version) to v10(2004 release version) |  | Engine/Renderer |  | All |
| md5animV6ToV10 | Convert md5anim v6(2002 E3 demo version) to v10(2004 release version) |  | Engine/Renderer |  | All |
| md5V6ToV10 | Convert md5mesh/md5anim v6(2002 E3 demo version) to v10(2004 release version) |  | Engine/Renderer |  | All |
| convertMd5Def | Convert other type animation model entityDef to md5mesh/md5anim |  | Engine/Renderer |  | All |
| cleanConvertedMd5 | Clean converted md5mesh/md5anim |  | Engine/Renderer |  | All |
| convertMd5AllDefs | Convert all other type animation models entityDef to md5mesh/md5anim |  | Engine/Renderer |  | All |
| convertImage | convert image format |  | Engine/Renderer |  | All |
| glConfig | print OpenGL config |  | Engine/Renderer | print glConfig variable | All |
| exportFont | Convert ttf/ttc font file to DOOM3 wide character font file |  | Engine/Renderer | require freetype2 | All |
| extractBimage | extract DOOM3-BFG's bimage image |  | Engine/Renderer | extract to TGA RGBA image files | All |
| idTech4AmmSettings | Show idTech4A++ new cvars and commands |  | Engine/Framework |  | All |
| botRunAAS | compiles an AAS file for a map for DOOM 3 multiplayer-game |  | Game/DOOM3 | Only for generate bot aas file if map has not aas file | All |
| addBot | adds a new bot |  | Game/DOOM3 | need SABotA7 files | All |
| removeBot | removes bot specified by id (0,15) |  | Game/DOOM3 | need SABotA7 files | All |
| addbots | adds multiplayer bots batch |  | Game/DOOM3 | need SABotA7 files | All |
| fillbots | fill bots to maximum of server |  | Game/DOOM3 | need SABotA7 files | All |
| removeBots | disconnect multi bots by client ID |  | Game/DOOM3 | need SABotA7 files | All |
| appendBots | append more bots(over maximum of server) |  | Game/DOOM3 | need SABotA7 files | All |
| cleanBots | disconnect all bots |  | Game/DOOM3 | need SABotA7 files | All |
| truncBots | disconnect last bots |  | Game/DOOM3 | need SABotA7 files | All |
| botLevel | setup all bot level |  | Game/DOOM3 | need SABotA7 files | All |
| botWeapons | setup all bot initial weapons |  | Game/DOOM3 | need SABotA7 files | All |
| botAmmo | setup all bot initial weapons ammo clip |  | Game/DOOM3 | need SABotA7 files | All |
| botRunAAS | compiles an AAS file for a map for Quake 4 multiplayer-game |  | Game/Quake4 | Only for generate bot aas file if map has not aas file | All |
| addBot | adds a new bot |  | Game/Quake4 | need SABotA9 files | All |
| removeBot | removes bot specified by id (0,15) |  | Game/Quake4 | need SABotA9 files | All |
| addbots | adds multiplayer bots batch |  | Game/Quake4 | need SABotA9 files | All |
| fillbots | fill bots to maximum of server |  | Game/Quake4 | need SABotA9 files | All |
| removeBots | disconnect multi bots by client ID |  | Game/Quake4 | need SABotA9 files | All |
| appendBots | append more bots(over maximum of server) |  | Game/Quake4 | need SABotA9 files | All |
| cleanBots | disconnect all bots |  | Game/Quake4 | need SABotA9 files | All |
| truncBots | disconnect last bots |  | Game/Quake4 | need SABotA9 files | All |
| botLevel | setup all bot level |  | Game/Quake4 | need SABotA9 files | All |
| botWeapons | setup all bot initial weapons |  | Game/Quake4 | need SABotA9 files | All |
| botAmmo | setup all bot initial weapons ammo clip |  | Game/Quake4 | need SABotA9 files | All |

----------------------------------------------------------------------------------

> ### About DOOM 3
##### Switch current weapon and last weapon  
```bind "Your key" "_impulse51"```

----------------------------------------------------------------------------------

> ### About Quake IV
##### For playing Quake 4([jmarshall](https://github.com/jmarshall23) 's [Quake4Doom](https://github.com/jmarshall23/Quake4Doom)). Now can play all levels, but some levels has bugs.  
1. Putting PC Quake 4 game data file to `q4base` folder and START directly.
2. *Effect system*: Quake4 new advanced `BSE` particle system is working now! Also see [Quake4BSE](https://github.com/jmarshall23/Quake4BSE), [Quake4Decompiled](https://github.com/jmarshall23/Quake4Decompiled), and OpenBSE with DOOM3 original FX/Particle system has been removed.

----------------------------------------------------------------------------------

> ### About Prey(2006)
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

> ### Screenshot
##### Game

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_bathroom.png" alt="Classic bathroom">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_bathroom_jill_stars.png" alt="Classic bathroom in Rivensin mod">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake4.png" alt="Quake IV on DOOM3">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_prey.png" alt="Prey(2006) on DOOM3">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg.jpg" alt="DOOM3 BFG">

##### Mod

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_roe.png" width="50%" alt="Resurrection of Evil"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_the_lost_mission.png" width="50%" alt="The lost mission">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_classic_doom3.png" width="50%" alt="Classic DOOM">

##### Other

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake_iii_arena.jpg" width="50%" alt="Quake III : Arena"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake_iii_team_arena.jpg" width="50%" alt="Quake III : Team Arena">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_return_to_castle_wolfenstein.jpg" width="50%" alt="Return to Castle Wolfenstein"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_the_dark_mod.jpg" width="50%" alt="The Dark Mod">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake2.jpg" width="50%" alt="Quake II"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake1.jpg" width="50%" alt="Quake I">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom1.jpg" width="33%" alt="DOOM 3 BFG: DOOM I"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom3.jpg" width="34%" alt="DOOM 3 BFG: DOOM III"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3bfg_doom2.jpg" width="33%" alt="DOOM 3 BFG: DOOM II">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_wolfenstein_enemy_territory.jpg" width="50%" alt="Wolfenstein: Enemy Territory"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_realrtcw.jpg" width="50%" alt="RealRTCW">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_openja.jpg" width="50%" alt="STAR WARS™ Jedi Knight - Jedi Academy™"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_openjo.jpg" width="50%" alt="STAR WARS™ Jedi Knight II - Jedi Outcast™">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_urt.jpg" width="50%" alt="Urban Terror"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_open.jpg" width="50%" alt="Serious Sam Classic: The Second Encounter">

----------------------------------------------------------------------------------

> ### Changes:

[Change logs](CHANGES.md ':include')

----------------------------------------------------------------------------------

> ### Build:
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

> ### Run idTech4A++ on other Android application with Android intent
1. Setup game type with `game` key: also see Q3E/com.n0n3m4.q3e.Q3EGameConstants.java GAME_XXX constants. Valid value: `doom3` `quake4` `prey2006` `quake2` `quake3` `rtcw` `tdm` `quake1` `doom3bfg` `gzdoom` `etw` `realrtcw` `fteqw` `openja` `openjo` `samtfe` `samtse` `urt` `openmohaa` `source`
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

> ### Bot support on DOOM3/Quake4
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

> ### Full-body-awareness on DOOM3/Quake4/Prey(2006)
1. Make full-body-awareness mode(view player model like first-person view)
[DOOM 3 example](Q3E/src/main/jni/doom3/base/full_body_awareness.cfg ':include')  
[Quake 4 example](Q3E/src/main/jni/doom3/q4base/full_body_awareness.cfg ':include')  
[Prey(2006) example](Q3E/src/main/jni/doom3/preybase/full_body_awareness.cfg ':include')  

2. Make third-person mode and use crosshair(different from `pm_thirdPerson`)
[DOOM 3 example](Q3E/src/main/jni/doom3/base/full_body_awareness_third_persion.cfg ':include')  
[Quake 4 example](Q3E/src/main/jni/doom3/q4base/full_body_awareness_third_persion.cfg ':include')  
[Prey(2006) example](Q3E/src/main/jni/doom3/preybase/full_body_awareness_third_persion.cfg ':include') 

----------------------------------------------------------------------------------

> ### Player view flashlight on DOOM3
[Point flashlight example](Q3E/src/main/jni/doom3/base/show_view_point_flashlight.cfg ':include')  
[Spot flashlight example](Q3E/src/main/jni/doom3/base/show_view_spot_flashlight.cfg ':include')  

----------------------------------------------------------------------------------

> ### Player body view on DOOM3/Quake4
[DOOM 3 example](Q3E/src/main/jni/doom3/base/def/player_viewbody.cfg ':include')  
[Quake 4 example](Q3E/src/main/jni/doom3/q4base/def/player_viewbody.cfg ':include')  
	
----------------------------------------------------------------------------------

> ### Branch:

##### master:
* /idTech4Amm: frontend source
* /Q3E /Q3E/src/main/jni/doom3: game source
* /CHECK_FOR_UPDATE.json: Check for update config JSON

##### free:
* For F-Droid pure free version.

##### package:
* /screenshot: screenshot pictures
* /source: Reference source
* /pak: Game resource

##### n0n3m4_original_old_version:
* Original old `n0n3m4` version source.

----------------------------------------------------------------------------------
> ### Extras download:

* [Baidu网盘: https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w 提取码: `pyyj`](https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w?pwd=pyyj)
* [Baidu贴吧: BEYONDK2000](https://tieba.baidu.com/p/6825594793)
* [F-Droid(different signature)](https://f-droid.org/packages/com.karin.idTech4Amm/)
----------------------------------------------------------------------------------
