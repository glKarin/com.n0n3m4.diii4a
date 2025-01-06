## idTech4A++ (Harmattan Edition)  
**idTech** engine games **For** **A**ndroid. An **idTech** games runtime libraries collection on Android  
#### DOOM III/Quake 4/Prey(2006) OpenGLES on Android/Windows/Linux  
##### DOOM 3 BFG/The Dark Mod/Quake 1 2 3/RTCW/GZDOOM/ETW/RealRTCW OpenGLES on Android  
#### 毁灭战士3/雷神之锤4/掠食(2006) 安卓/Windows/Linux OpenGLES移植版  
##### 毁灭战士3 BFG/The Dark Mod/雷神之锤1 2 3/重返德军总部/GZDOOM/深入敌后: 德军总部/真·重返德军总部 安卓OpenGLES移植版  
##### Original named DIII4A++, based on com.n0n3m4.diii4a's OpenGLES version.
**Latest version:**
1.1.0harmattan59(natasha)  
**Latest update:**
2024-11-20  
**Arch:**
arm64 armv7-a  
**Platform:**
Android 4.4+  
**License:**
GPLv3  

----------------------------------------------------------------------------------
> **idTech4's feature**  
* Linux/Windows(MinGW/MSVC(without editor)) build
* multi-threading renderer
* pure soft shadow with shadow-mapping
* soft shadow with stencil-shadow and translucent stencil shadow
* lighting model: Phong/Blinn-phong/PBR/Ambient/No-lighting
* debug render tools support with programming render pipeline
* OpenGLES2.0/OpenGLES3.0
* png/dds texture image, jpeg/png/bmp/dds format of screenshot
* obj/dae format static model
* OpenAL(soft) and EFX Reverb
* DOOM3(with full body awareness mod)
* Quake4(with bot mod, full body awareness mod) and Raven's idTech4 engine
* Prey(2006)(with full body awareness mod) and HumanHead's idTech4 engine

> **Build DOOM3/Quake4/Prey(2006) mod for Android idTech4A++**  

[DOOM 3 & RoE SDK](https://github.com/glKarin/idtech4amm_doom3_sdk)  
[Quake 4 SDK](https://github.com/glKarin/idtech4amm_quake4_sdk)  
[Prey(2006) SDK](https://github.com/glKarin/idtech4amm_prey_sdk)  

> **Compare with other OpenGLES rendering version of DOOM3**  

| Feature | idTech4A++ | Other |
|:-----|:-----:|:-----:|
| Multi-threading | Support<br/>(but can't switch in gaming) | d3es-multithread support<br/>(and support switch in gaming) |
| New stage shader<br/>(heatHaze, heatHazeWithMask, heatHazeWithMaskAndVertex, colorProcess) | Yes | - |
| TexGen shader | Yes | - |
| Shadow mapping for pure soft shadow | Yes | - |
| Soft/Translucent stencil shadow | Yes<br/>(Soft stencil shadow only support on OpenGLES3.1+) | - |
| OpenGL ES version | 2.0 and 3.0+<br/>(point light shadow mapping shader use cubemap on OpenGLES2.0, use texture array on OpenGLES3.0+) | 2.0(3.0+ compat) |
| No lighting | Yes<br/>(And support switch in gaming by set harm_r_lightingModel to 0) | Yes |
| Debug render tools | Yes<br/>(need to set harm_r_renderToolsMultithread to 1 if with multi-threading) | - |
| PBR lighting model | Yes<br/>(using [idtech4_pbr](https://github.com/jmarshall23/idtech4_pbr). But specular textures are not RMAO format in idTech4 game data) | - |

> **Support games**

| Game | Engine | Version | OpenGL ES version | Mods |
|:-----|:-----:|:-----:|:-----:|:-----:|
| DOOM III | n0n3m4's dante | - | 2.0/3.0 | [Resurrection of Evil]()<br/>[The Lost Mission](https://www.moddb.com/mods/the-lost-mission)<br/>[Classic DOOM3](https://www.moddb.com/mods/classic-doom-3)<br/>[Rivensin](https://www.moddb.com/mods/ruiner)<br/>[HardCorps](https://www.moddb.com/mods/hardcorps)<br/>[Overthinked Doom^3](https://www.moddb.com/mods/overthinked-doom3)<br/>[Sabot(a7x)](https://www.moddb.com/games/doom-3-resurrection-of-evil/downloads/sabot-alpha-7x)<br/>[HeXen:Edge of Chaos](https://www.moddb.com/mods/hexen-edge-of-chaos)<br/>[Fragging Free](https://www.moddb.com/mods/fragging-free)<br/>[LibreCoop](https://www.moddb.com/mods/librecoop-dhewm3-coop)<br/>[LibreCoop D3XP](https://www.moddb.com/mods/librecoop-dhewm3-coop)<br/>[Perfected Doom 3](https://www.moddb.com/mods/perfected-doom-3-version-500)<br/>[Perfected Doom 3:RoE](https://www.moddb.com/mods/perfected-doom-3-version-500)<br/>[Doom 3: Phobos](https://www.moddb.com/mods/phobos)([Dhewm3 compatibility patch](https://www.moddb.com/games/doom-iii/addons/doom-3-phobos-dhewm3-compatibility-patch)) |
| Quake IV | n0n3m4's dante | - | 2.0/3.0 | [Hardqore](https://www.moddb.com/mods/quake-4-hardqore) |
| Prey(2006) | n0n3m4's dante | - | 2.0/3.0 |  |
| DOOM 3 BFG(Classic DOOM 1&2) | [RBDOOM-3-BFG](https://github.com/RobertBeckebans/RBDOOM-3-BFG) | 1.4.0<br/>(The last OpenGL renderer version) | 3.0 |  |
| The Dark Mod | [Dark Mod](https://www.thedarkmod.com) | 2.12 | 3.2<br/>(require geometry shader support) |  |
| Return to Castle Wolfenstein | [iortcw](https://github.com/iortcw/iortcw) | - | 1.1 |  |
| Quake III Arena | [ioquake3](https://github.com/ioquake/ioq3) | - | 1.1 | Quake III Team Arena |
| Quake II | [Yamagi Quake II](https://github.com/yquake2/yquake2) | - | 1.1/3.2 | ctf<br/>rogue<br/>xatrix<br/>zaero |
| Quake I | [Darkplaces](https://github.com/DarkPlacesEngine/darkplaces) | - | 2.0 |  |
| GZDOOM | [GZDOOM](https://github.com/ZDoom/gzdoom) 64bits | - | 3.2 |  |
| Wolfenstein: Enemy Territory | [ET: Legacy](https://www.etlegacy.com) Omni-Bot support | 2.8.1 | 1.1 |  |
| RealRTCW | [RealRTCW](https://github.com/wolfetplayer/RealRTCW) | 5.0 | 1.1 |  |

[<img src="https://fdroid.gitlab.io/artwork/badge/get-it-on.png"
     alt="Get it on F-Droid"
     height="80">](https://f-droid.org/packages/com.karin.idTech4Amm/)

Or download the latest APK from the [Releases Section](https://github.com/glKarin/com.n0n3m4.diii4a/releases/latest).
Tag with `-free` only for F-Droid update.

| Feature            | Github | F-Droid  |
|:-------------------|:------:|:--------:|
| Ouya TV            |   Yes  |    No    |
| Source code in apk |   Yes  |    No    |

> Testing version(Non-release. Automatic CI building By Github actions when pushing commits)
* idTech4A++ for Android: [Android testing](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/android_testing).
* DOOM 3/Quake 4/Prey(2006) for Windows x64: [Windows x64 testing](https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/windows_x64_testing).

----------------------------------------------------------------------------------
### Update

* Support `Omni-Bot` in Wolfenstein: Enemy Territory.
* Fix rendering on Mali GPU in DOOM 3-BFG.
* Fix rendering on Mali GPU in The Dark Mod.
* Fix stencil shadow with `cg_shadows` = 2 in Wolfenstein: Enemy Territory.
* Support choose a mod directory in GZDOOM.
* Add some new features options on launcher in Wolfenstein: Enemy Territory, RealRTCW, DOOM3-BFG, Quake 2, GZDOOM, The Dark Mod.
* Add use high precision float on GLSL shaders(cvar `harm_r_useHighPrecision`) in DOOM 3/Quake 4/Prey.
* Add 5 onscreen buttons.
* Add `Phobos(for Dhewm3)` mod of DOOM3 support, game data directory named `tfphobos`(d3xp and dhewm3 compatibility patch required). More view in [Doom 3: Phobos](https://www.moddb.com/mods/phobos) and [Doom 3: Phobos - dhewm3 compatibility patch](https://www.moddb.com/games/doom-iii/addons/doom-3-phobos-dhewm3-compatibility-patch).

----------------------------------------------------------------------------------

* 深入敌后:德军总部新增`Omni-Bot`支持.
* 毁灭战士3-BFG修复在Mali GPU上的渲染问题.
* The Dark Mod修复在Mali GPU上的渲染问题.
* 修复深入敌后:德军总部的模板阴影当`cg_shadows` = 2.
* GZDOOM选择mod时支持选择整个文件夹.
* 启动器新增一些新特性的设置功能: 深入敌后:德军总部, 真·重返德军总部, 毁灭战士3-BFG, 雷神之锤2, GZDOOM, The Dark Mod.
* 毁灭战士3/雷神之锤 4/掠食(2006)支持GLSL着色器中使用高精度浮点数(cvar `harm_r_useHighPrecision`).
* 新增5个虚拟按键.

----------------------------------------------------------------------------------
### Standalone game directory
  Because more support games, it cause all game mods data directory put on a shared folder, them maybe have same name, and diffcult to view/manage. So application default enable `Standalone game directory` since version 1.1.0harmattan57, and you can also disable it on launcher settings.
  
  If enable `Standalone game directory`, game data directory should put on `Standalone game directory` itself(e.g.).
  
> Games of `Standalone game directory` and folder name:
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
  
> Games of always force `Standalone game directory`:
* **The Dark Mod**: darkmod/
* **GZDOOM**: gzdoom/

----------------------------------------------------------------------------------

##### idTech4's new Cvar
###### Flag
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
| harm_r_multithread | Bool | 1 | Multithread backend | INIT |  | Engine/Renderer | setup with launcher on Android | Windows, Linux |
| harm_r_clearVertexBuffer | Integer | 2 | Clear vertex buffer on every frame | ARCHIVE, FIXED | 0, 1, 2 | Engine/Renderer | 0 = not clear(original);<br/> 1 = only free VBO memory;<br/> 2 = free VBO memory and delete VBO handle(only without multi-threading, else same as 1) | All |
| harm_r_maxAllocStackMemory | Integer | 524288 | Control allocate temporary memory when load model data | ARCHIVE |  | Engine/Renderer | For load large model, because stack memory is limited on OS.<br/> 0 = Always heap;<br/> Negative = Always stack;<br/> Positive = Max stack memory limit(If less than this `byte` value, call `alloca` in stack memory, else call `malloc`/`calloc` in heap memory) | All |
| harm_r_shaderProgramDir | String |  | Setup external OpenGLES2 GLSL shader program directory path | ARCHIVE |  | Engine/Renderer | empty is glslprogs(default) | All |
| harm_r_shaderProgramES3Dir | String |  | Setup external OpenGLES3 GLSL shader program directory path | ARCHIVE |  | Engine/Renderer | empty is glsl3progs(default) | All |
| harm_r_shadowCarmackInverse | Bool | 0 | Stencil shadow using Carmack-Inverse | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_lightingModel | Integer | 1 | Lighting model when draw interactions | ARCHIVE | 0, 1, 2, 3 | Engine/Renderer | 1 = Phong(default);<br/> 2 = Blinn-Phong;<br/> 3 = PBR;<br/> 0 = Ambient(no lighting) | All |
| harm_r_specularExponent | Float | 3.0 | Specular exponent in Phong interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_specularExponentBlinnPhong | Float | 12.0 | Specular exponent in Blinn-Phong interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_specularExponentPBR | Float | 5.0 | Specular exponent in PBR interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_normalCorrectionPBR | Float | 1.0 | Vertex normal correction in PBR interaction lighting model | ARCHIVE | [0.0 - 1.0] | Engine/Renderer | 1 = pure using bump texture;<br/> 0 = pure using vertex normal;<br/> 0.0 - 1.0 = bump texture * harm_r_normalCorrectionPBR + vertex normal * (1 - harm_r_normalCorrectionPBR) | All |
| r_maxFps | Integer | 0 | Limit maximum FPS. | ARCHIVE | &gt;= 0 | Engine/Renderer | 0 = unlimited | All |
| r_screenshotFormat | Integer | 0 | Screenshot format | ARCHIVE | 0, 1, 2, 3, 4 | Engine/Renderer | 0 = TGA (default),<br/> 1 = BMP,<br/> 2 = PNG,<br/> 3 = JPG,<br/> 4 = DDS | All |
| r_screenshotJpgQuality | Integer | 75 | Screenshot quality for JPG images | ARCHIVE | [0 - 100] | Engine/Renderer |  | All |
| r_screenshotPngCompression | Integer | 3 | Compression level when using PNG screenshots | ARCHIVE | [0 - 9] | Engine/Renderer |  | All |
| r_useShadowMapping | Bool | 0 | use shadow mapping instead of stencil shadows | ARCHIVE | All | Engine/Renderer |  | All |
| harm_r_shadowMapAlpha | Float | 1.0 | shadow's alpha in shadow mapping | ARCHIVE | [0.0 - 1.0] | Engine/Renderer |  | All |
| harm_r_shadowMapJitterScale | Float | 2.5 | scale factor for jitter offset | ARCHIVE |  | Engine/Renderer |  | All |
| r_forceShadowMapsOnAlphaTestedSurfaces | Bool | 0 | render performed material shadow in shadow mapping | ARCHIVE, ISSUE |  | Engine/Renderer | 0 = same shadowing as with stencil shadows,<br/> 1 = ignore noshadows for alpha tested materials | All |
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
| harm_r_autoAspectRatio | Integer | 1 | automatic setup aspect ratio of view | ARCHIVE | 0, 1, 2 | Engine/Renderer | 0 = manual<br/>1 = force setup r_aspectRatio to -1<br/>2 = automatic setup r_aspectRatio to 0,1,2 by screen size | Android |
| harm_r_renderToolsMultithread | Bool | 0 | Enable render tools debug with GLES in multi-threading | ARCHIVE |  | Engine/Renderer |  | All |
| r_useETC1 | Bool | 0 | use ETC1 compression | INIT |  | Engine/Renderer |  | All |
| r_useETC1cache | Bool | 0 | use ETC1 compression | INIT |  | Engine/Renderer |  | All |
| r_useDXT | Bool | 0 | use DXT compression if possible | INIT |  | Engine/Renderer |  | All |
| r_useETC2 | Bool | 0 | use ETC2 compression instead of RGBA4444 | INIT |  | Engine/Renderer | Only for OpenGLES3.0+ | All |
| r_noLight | Bool | 0 | lighting disable hack | INIT |  | Engine/Renderer | 1 = disable lighting(not allow switch, must setup on command line) | All |
| harm_r_useHighPrecision | Bool | Android = 0; Other = 1 | Use high precision float on GLSL shade | INIT |  | Engine/Renderer |  | All |
| harm_fs_gameLibPath | String |  | Setup game dynamic library | ARCHIVE |  | Engine/Framework |  | Android |
| harm_fs_gameLibDir | String |  | Setup game dynamic library directory path | ARCHIVE |  | Engine/Framework |  | Android |
| harm_com_consoleHistory | Integer | 2 | Save/load console history | ARCHIVE | 0, 1, 2 | Engine/Framework | 0 = disable;<br/> 1 = loading in engine initialization, and saving in engine shutdown;<br/> 2 = loading in engine initialization, and saving in every e executing | All |
| r_scaleMenusTo43 | Bool | 0 | Scale menus, fullscreen videos and PDA to 4:3 aspect ratio | ARCHIVE |  | Engine/GUI |  | All |
| s_driver | String | AudioTrack | sound driver | ARCHIVE | AudioTrack, OpenSLES | Engine/Sound | sound if without OpenAL on Android | Android |
| harm_s_OpenSLESBufferCount | Integer | 3 | Audio buffer count for OpenSLES | ARCHIVE | &gt;= 3 | Engine/Sound | min is 3, only for if without OpenAL and use OpenSLES on Android | Android |
| harm_s_useAmplitudeDataOpenAL | Bool | 0 | Use amplitude data on OpenAL | DISABLE, ISSUE |  | Engine/Sound | It cause large shake | All |
| harm_in_smoothJoystick | Bool | 0 | Enable smooth joystick |  |  | Engine/Input | Automatic setup initial value by Android layer | Android |
| harm_g_normalizeMovementDirection | Integer | -1 | Re-normalize player/walker movement direction | ARCHIVE | -1, 0, &gt; 0 | Game/DOOM3 | 0 = disable; -1 = automcatic; &gt;0 = max degree | Android |
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset(&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;) | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | Head | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_showViewBody | Bool | 0 | Show view body | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessFirstPerson | Bool | 0 | Enable first person view in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_doubleJump | Bool | 0 | Enable double-jump | ARCHIVE |  | Game/DOOM3/Rivensin |  | All |
| harm_pm_autoForceThirdPerson | Bool | 1 | Force set third person view after game level load end | ARCHIVE |  | Game/DOOM3/Rivensin |  | All |
| harm_pm_preferCrouchViewHeight | Float | 32 | Set prefer crouch view height in Third-Person | ARCHIVE | &gt;= 0 | Game/DOOM3/Rivensin | suggest 32 - 39, less or equals 0 to disable | All |
| harm_g_normalizeMovementDirection | Integer | -1 | Re-normalize player/walker movement direction | ARCHIVE | -1, 0, &gt; 0 | Game/Quake4 | 0 = disable; -1 = automcatic; &gt;0 = max degree | Android |
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset(&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;) | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | head_channel | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessFirstPerson | Bool | 0 | Enable first person view in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_ui_showViewBody | Bool | 0 | Show view body | ARCHIVE |  | Game/Quake4 |  | All |
| harm_g_autoGenAASFileInMPGame | Bool | 1 | For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic | ARCHIVE |  | Game/Quake4 |  | All |
| harm_g_vehicleWalkerMoveNormalize | Bool | 1 | Re-normalize vehicle walker movement | ARCHIVE |  | Game/Quake4 | Only for smooth jotstick on DIII4A | Android |
| harm_gui_defaultFont | String | chain | Default font name | ARCHIVE | chain, lowpixel, marine, profont, r_strogg, strogg | Engine/Quake4/GUI | It will be available in next running | All |
| harm_si_autoFillBots | Bool | 1 | Automatic fill bots after map loaded in multiplayer game | ARCHIVE | &gt;=0 | Game/Quake4 | 0 = disable; other number = bot num | All |
| harm_si_botLevel | Integer | 0 | Bot level | ARCHIVE | [0 - 8] | Game/Quake4 | 0 = auto; 1 - 8 = difficult level | All |
| harm_g_mutePlayerFootStep | Bool | 0 | Mute player's footstep sound | ARCHIVE |  | Game/Quake4 |  | All |
| harm_g_normalizeMovementDirection | Integer | -1 | Re-normalize player/walker movement direction | ARCHIVE | -1, 0, &gt; 0 | Game/Prey | 0 = disable; -1 = automcatic; &gt;0 = max degree | Android |
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset(&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;) | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | neck | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_ui_translateAlienFont | String | fonts | Setup font name for automatic translate `alien` font text of GUI | ARCHIVE | fonts, fonts/menu, "" | Engine/Prey/GUI | empty to disable | All |
| harm_ui_translateAlienFontDistance | Float | 200 | Setup max distance of GUI to view origin for enable translate `alien` font text | ARCHIVE |  | Engine/Prey/GUI | 0 = disable;<br/> -1 = always;<br/> positive: distance value | All |
| harm_ui_subtitlesTextScale | Float | 0.32 | Subtitles's text scale | ARCHIVE |  | Engine/Prey/GUI | &lt;= 0 to unset | All |
| harm_r_skipHHBeam | Bool | 0 | Skip beam model render | DEBUG |  | Engine/Prey/Renderer |  | All |

----------------------------------------------------------------------------------

##### idTech4's new command

| Command | Description | Usage | Scope | Remark | Platform |
|:---|:---|:---|:---|:---|:---:|
| exportGLSLShaderSource | export GLSL shader source to filesystem |  | Engine/Renderer | Only export shaders of using OpenGLES2.0 or OpenGLES3.0 | All |
| printGLSLShaderSource | print internal GLSL shader source |  | Engine/Renderer | Only print shaders of using OpenGLES2.0 or OpenGLES3.0 | All |
| exportDevShaderSource | export internal original C-String GLSL shader source for developer |  | Engine/Renderer | Export all shaders of OpenGLES2.0 and OpenGLES3.0 | All |
| reloadGLSLprograms | reloads GLSL programs |  | Engine/Renderer |  | All |
| convertImage | convert image format |  | Engine/Renderer |  | All |
| r_multithread | print multi-threading state |  | Engine/Renderer | Only for tell user r_multithread is not a cvar | All |
| glConfig | print OpenGL config |  | Engine/Renderer | print glConfig variable | All |
| botRunAAS | compiles an AAS file for a map for Quake 4 multiplayer-game |  | Game/Quake4 | Only for generate bot aas file if map has not aas file | All |
| addBot | adds a new bot |  | Game/Quake4 | need SABotA9 files | All |
| removeBot | removes bot specified by id (0,15) |  | Game/Quake4 | need SABotA9 files | All |
| addbots | adds multiplayer bots batch |  | Game/Quake4 | need SABotA9 files | All |
| fillbots | fill bots |  | Game/Quake4 | need SABotA9 files | All |
| sabot | debug SaBot info |  | Game/Quake4 | need SABotA9 files | All |

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
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_wolfenstein_enemy_territory.jpg" width="50%" alt="Wolfenstein: Enemy Territory">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_realrtcw.jpg" width="50%" alt="RealRTCW">

----------------------------------------------------------------------------------

### Changes:

[Change logs](CHANGES.md ':include')

----------------------------------------------------------------------------------

### Build:
* idTech4A++ using std libc's malloc/free in Mem_Alloc/Mem_Free in idlib/Heap.cpp
* idTech4A++ force using generic SIMD, not compile all SIMD of processor in all platform(TODO: enable them on windows/Linux)

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
> 10. _STENCIL_SHADOW_IMPROVE: Add stencil shadow improve support(translucent shadow, force combine global shadow and self local shadow).
> 11. _SOFT_STENCIL_SHADOW: soft shadow(OpenGLES3.1+), must defined `_STENCIL_SHADOW_IMPROVE` first.
> 12. _MINIZ: Using miniz instead of zlib, using minizip instead of DOOM3's UnZip(Disabled).
> 13. _USING_STB_OGG: Using stb_vorbis instead of libogg and libvorbis(Disabled).

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
Define macro `_MOD_VIEW_BODY` will compile view-body support.

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
> 2. ./cmake_linux_build_doom3_quak4_prey.sh

#### Windows(MSVC)
> 1. REQUIRE SDL2, cURL, zlib: vcpkg install SDL2 curl
> 2. Setup your vcpkg.cmake path
> 3. cmake_msvc_build_doom3_quak4_prey.bat
> 4. Copy OpenAL32.dll from vcpkg package path to binary path: vcpkg install OpenAL-Soft

----------------------------------------------------------------------------------

### Player body view in DOOM3/Quake4
> 1. Declaration player_viewbody
```
entityDef player_viewbody { // default name is player_viewbody, or setup in player entity with property 'player_viewbody'
    "spawnclass"				"idViewBody"
    "body_model"                "player_model_torso_and_lower_body" // body's md5 model: animations's name same as player model: string
    "body_offset"               "-15 0 0" // extras model offset: vector <forward right up>, default = 0 0 0
    "body_allChannel"			"0" // play animation with all channels, else only play with legs channel: bool, default = 1
    "body_usePlayerModel"		"0" // use player model and not use 'body_model': bool, default = 0
    // "anim run_forward" 		"walk_forward" // override model animation name: string, "anim <model animation name>" "<replace animation name>"
	
	// Hide surface only for Quake4
    "body_hidesurfaces" 		"shader_head,shader_toast" // hide surface names, separate by ',': string
    "hidesurface1" 			"shader_arm" // hide surface name by entity property: string, hidesurfaceXXX
    "hidesurface5" 			"shader_neck"
    "hidesurface6" 			"shader_hand"
}
```
> 2. DOOM3's player view body model declaration(animations name same as idPlayer's model)
```
// DOOM3 view body model example:
model player_model_torso_and_lower_body {
    offset (0 0 0)
    model models/md5/player_model_torso_and_lower_body.md5mesh

// loop anims:
    idle models/md5/player_model_torso_and_lower_body/idle.md5anim
    run_forward models/md5/player_model_torso_and_lower_body/run_forward.md5anim
    run_backwards models/md5/player_model_torso_and_lower_body/run_backwards.md5anim
    run_strafe_left models/md5/player_model_torso_and_lower_body/run_strafe_left.md5anim
    run_strafe_right models/md5/player_model_torso_and_lower_body/run_strafe_right.md5anim
    walk models/md5/player_model_torso_and_lower_body/walk.md5anim
    walk_backwards models/md5/player_model_torso_and_lower_body/walk_backwards.md5anim
    walk_strafe_left models/md5/player_model_torso_and_lower_body/walk_strafe_left.md5anim
    walk_strafe_right models/md5/player_model_torso_and_lower_body/walk_strafe_right.md5anim
    crouch models/md5/player_model_torso_and_lower_body/crouch.md5anim
    crouch_walk models/md5/player_model_torso_and_lower_body/crouch_walk.md5anim
    crouch_walk_backwards models/md5/player_model_torso_and_lower_body/crouch_walk_backwards.md5anim
    fall models/md5/player_model_torso_and_lower_body/fall.md5anim

// single anims:
    crouch_down models/md5/player_model_torso_and_lower_body/crouch_down.md5anim
    crouch_up models/md5/player_model_torso_and_lower_body/crouch_up.md5anim
    run_jump models/md5/player_model_torso_and_lower_body/run_jump.md5anim
    jump models/md5/player_model_torso_and_lower_body/jump.md5anim
    hard_land models/md5/player_model_torso_and_lower_body/hard_land.md5anim
    soft_land models/md5/player_model_torso_and_lower_body/soft_land.md5anim
}
```
> 3. Quake4's player view body model declaration(animations name same as idPlayer's model)
```
// Quake4 view body model example:
model player_model_torso_and_lower_body {
    offset (0 0 0)
    model models/md5/player_model_torso_and_lower_body.md5mesh

// loop anims:
    idle models/md5/player_model_torso_and_lower_body/idle.md5anim
    run_forward models/md5/player_model_torso_and_lower_body/run_forward.md5anim
    run_backwards models/md5/player_model_torso_and_lower_body/run_backwards.md5anim
    run_strafe_left models/md5/player_model_torso_and_lower_body/run_strafe_left.md5anim
    run_strafe_right models/md5/player_model_torso_and_lower_body/run_strafe_right.md5anim
    walk_forward models/md5/player_model_torso_and_lower_body/walk_forward.md5anim
    walk_backwards models/md5/player_model_torso_and_lower_body/walk_backwards.md5anim
    walk_left models/md5/player_model_torso_and_lower_body/walk_left.md5anim
    walk_right models/md5/player_model_torso_and_lower_body/walk_right.md5anim
    crouch models/md5/player_model_torso_and_lower_body/crouch.md5anim
    crouch_walk models/md5/player_model_torso_and_lower_body/crouch_walk.md5anim
    crouch_walk_backward models/md5/player_model_torso_and_lower_body/crouch_walk_backward.md5anim
    fall models/md5/player_model_torso_and_lower_body/fall.md5anim

// single anims:
    crouch_down models/md5/player_model_torso_and_lower_body/crouch_down.md5anim
    crouch_up models/md5/player_model_torso_and_lower_body/crouch_up.md5anim
    run_jump models/md5/player_model_torso_and_lower_body/run_jump.md5anim
    jump models/md5/player_model_torso_and_lower_body/jump.md5anim
    hard_land models/md5/player_model_torso_and_lower_body/hard_land.md5anim
    soft_land models/md5/player_model_torso_and_lower_body/soft_land.md5anim
}
```

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

### Open source licence
> Game engine
* DOOM 3: GPLv3
* Quake4 SDK
* Prey SDK
* RBDOOM-3-BFG
* TheDarkMod
* ioq3
* iortcw
* yQuake2
* Darkplaces
* GZDOOM
* ETLegacy
* RealRTCW
> Library(Source)
* cJSON 
* curl: The curl license
* etc2comp
* flac: GNU Free Documentation License / GNU GENERAL PUBLIC LICENSE Version 2 / GNU LESSER GENERAL PUBLIC LICENSE Version 2.1 / BSD-3-Clause license
* fluidsynth
* freetype: The FreeType License / The GNU General Public License version 2
* iconv
* irrxml
* libjpeg
* libogg
* libpng
* libsndfile
* libvorbis
* lua
* mbedtls
* miniz
* minizip
* mp3lame
* mpg123
* oboe
* Omni-Bot
* openal-soft
* openssl
* opus
* pugixml
* soil
* sqlite
* stb
> Library(Binary)
* ffmpeg-mobile
----------------------------------------------------------------------------------
