## Change logs

----------------------------------------------------------------------------------

> 1.1.0harmattan68 (2025-08-09)

* Update Wolfenstein: Enemy Territory(ET: Legacy) version to 2.83.2, fix Omni-bot initialization.
* Fix game controller, and add game controller configure on launcher `CONTROLS` tab.
* Add more alphabet on-screen buttons, and add `cs16-client` and `source-engine` on-screen button theme.
* Add load external game library support on Xash3D.
* Add built-in soft keyboard support.
* Update RealRTCW version to 5.2, version 5.1 will be removed on next release.

----------------------------------------------------------------------------------

> 1.1.0harmattan67 (2025-05-08)

* Add Serious Sam Classic: The First Encounter(game standalone directory named `serioussamtfe`) and Serious Sam Classic: The Second Encounter(game standalone directory named `serioussamtse`) support. More view in [SeriousSamClassic](https://github.com/tx00100xt/SeriousSamClassic).
* Add Xash3D and Counter-Strike 1.6(need extract CS1.6 extras resource on launcher first) support, game standalone directory named `xash3d`, more view in [xash3d](https://github.com/FWGS/xash3d-fwgs) and [cs16-client](https://github.com/Velaron/cs16-client).

----------------------------------------------------------------------------------

> 1.1.0harmattan66 (2025-06-25)

* Add global illumination rendering for make scene brighter(cvar harm_r_globalIllumination and harm_r_globalIlluminationBrightness) at launcher on DOOM 3/Quake 4/Prey(2006).
* Add built-in bot assets if external assets missing(so only require external aas files, defs/scripts files not required any longer, also extract aas files patch by launcher), add `+set harm_g_botEnableBuiltinAssets 1` to command or setup at launcher for enable on DOOM 3/Quake 4.
* Add retro postprocess rendering from RBDOOM-3-BFG 1.6.0(cvar r_renderMode) at launcher on DOOM 3/Quake 4/Prey(2006).
* Fix incorrect shadow rendering on perforated surfaces(cvar r_forceShadowMapsOnAlphaTestedSurfaces) with shadow mapping on DOOM 3.

----------------------------------------------------------------------------------

> 1.1.0harmattan65 (2025-05-05)

* Add cascaded shadow mapping with parallel lights(cvar `r_shadowMapSplits`) in OpenGL ES3 on DOOM 3/Quake 4/Prey(2006).
* Fix makron remote render at screen for texfure/cameraView1 in map game/core1 on Quake 4.
* Add spiritview and deathview GLSL shaders on Prey(2006).
* Update GZDOOM version to 4.14.1.
* Add multiplayer game bot support(need extract `d3_sabot_a7.pk4` patch resource first) on DOOM 3.
* Add player view flashlight with cvar `harm_ui_showViewLight`(include cvar: harm_ui_viewLightShader, harm_ui_viewLightRadius, harm_ui_viewLightOffset, harm_ui_viewLightType, harm_ui_viewLightOnWeapon), support bind to key for toggle(e.g. bind "g" "_impulse52") on DOOM 3.
* Support vsync setting on all OpenGL game.
* Improve multiplayer game bot system on Quake 4.
* Add skip player hit effect support with cvar `harm_g_skipHitEffect` and command `skipHitEffect` on DOOM 3/Quake 4/Prey(2006).
* Add enviroSuit player view vision GLSL shader on DOOM 3: RoE.
* Custom GLSL shaders using high precision(for fix blood pool in map game/waste) on Quake 4.
* Allow fire when focus NPC with cvar `harm_g_allowFireWhenFocusNPC` on Quake 4.
* Add custom GLSL shader program of new stage material support(keyword is `programGLSL`, e.g. programGLSL yourShaderInglProgs) on DOOM 3/Quake 4/Prey(2006).
* Fix save game path on FTEQW.
* On-screen buttons using OpenGL buffer.
* RealRTCW(ver 5.0) and The Dark Mod(2.12) have removed on this release!

----------------------------------------------------------------------------------

> 1.1.0harmattan63 (2025-04-01)

* The Dark Mod update to version 2.13. More view in [The Dark Mod(2.13)](https://www.thedarkmod.com/posts/the-dark-mod-2-13-has-been-released/).
* Add wide-character language support and DOOM3-BFG new fonts support on DOOM 3/Quake 4/Prey(2006). Only support UTF-8(BOM) encoding translation file.
* Add Chinese translation patch on DOOM3. First extract `DOOM3 Chinese translation(by 3DM DOOM3-BFG Chinese patch)`, then ckecked `Use DOOM3-BFG fonts instead of old fonts and enable wide-character language support` on launcher, finally add ` +set sys_lang chinese` to command line.
* Fix some errors and light gem(so reset cvar `tdm_lg_weak` value to 0) on The Dark Mod(2.12 and 2.13).
* Add DOOM3-BFG occlusion culling with cvar `harm_r_occlusionCulling` on DOOM 3/Quake 4/Prey(2006).
* Add combine shadow mapping option with cvar `harm_r_shadowMapCombine`(default 1, like now) on DOOM 3/Quake 4/Prey(2006).
* Fix some lights missing on ceil in map game/airdefense1 on Quake 4.
* Add game portal support on Prey(2006).
* Fix wrong resurrection position from deathwalk state when load game after restart application on Prey(2006).
* Fix key binding tips UI on Prey(2006).
* Using libogg/libvorbis instead of stb-vorbis of version 62 again on DOOM 3/Quake 4/Prey(2006).
* Support game data folder creation with `Game path tips` button on launcher `General` tab.
* [Warning]: RealRTCW(ver 5.0) and The Dark Mod(2.12) will be removed on next release in the future!

----------------------------------------------------------------------------------

> 1.1.0harmattan62 (2025-03-11)

* Add `OpenJK` support, [STAR WARS™ Jedi Knight - Jedi Academy™](https://store.steampowered.com/app/6020/STAR_WARS_Jedi_Knight__Jedi_Academy/) game standalone directory named `openja`, [STAR WARS™ Jedi Knight II - Jedi Outcast™](https://store.steampowered.com/app/6030/STAR_WARS_Jedi_Knight_II__Jedi_Outcast/) game standalone directory named `openjo`. More view in [OpenJK](https://github.com/JACoders/OpenJK).
* Update version to 5.1 on RealRTCW, support Survival mode. And version 5.0 is keeping until version 5.1 is stable.
* Fix audio playing when disable OpenAL on Quake3/ETW/RTCW/RealRTCW.
* Add Vulkan renderer backend on Quake2.
* Fix create render context on GZDOOM, it will use OpenGL ES if Vulkan initialization fail.
* Add `Open menu` button at end of `General` tab on launcher.

----------------------------------------------------------------------------------

> 1.1.0harmattan61 (2025-02-27)

* Add `FTEQW` support, game standalone directory named `fteqw`, support HeXen II, Half-Life [FreeHL](https://github.com/eukara/freehl), Counter Striker 1.5 [FreeCS](https://github.com/eukara/freecs). More view in [FTEQW](https://www.fteqw.org).
* DOOM 3 BFG add Vulkan renderer backend.
* Don't package source code to apk since version 61.

----------------------------------------------------------------------------------

> 1.1.0harmattan60 (2025-01-16)

* Support setup max game console height percentage(0 or 100 means not limit) on launcher `General` tab.
* Update GZDOOM version to 4.14.0.
* GZDOOM add Vulkan and OpenGL renderer backend.
* Update Wolfenstein: Enemy Territory(ET: Legacy) version to 2.83.1.
* Update Quake 1(Darkplaces) version.
* Update Quake 2(yquake2) version.
* Add use multisamples config in game.

----------------------------------------------------------------------------------

> 1.1.0harmattan59 (2024-11-20)

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

> 1.1.0harmattan58 (2024-10-29)

* Add `RealRTCW`(ver 5.0) support, game standalone directory named `realrtcw`, game data directory named `Main`. More view in [RealRTCW](https://github.com/wolfetplayer/RealRTCW).
* Fix light bar indicator of player's HUD by `darkness257` on The Dark Mod, now setup `tdm_lg_weak` to 1 automatically. More view in [The Darkmod light bar indicator bug](https://github.com/glKarin/com.n0n3m4.diii4a/issues/244).
* Support create desktop shortcut for games or current command on `Option` menu of launcher.
* Fix stencil shadow with `cg_shadows` = 2 on Quake 3.
* Improve stencil shadow with `cg_shadows` = 2 on RealRTCW.

----------------------------------------------------------------------------------

> 1.1.0harmattan57 (2024-10-01)

* Add `Wolfenstein: Enemy Territory` support, game standalone directory named `etw`, game data directory named `etmain` and `legacy`. More view in [ET: Legacy](https://www.etlegacy.com).
* Add `Quake 4: Hardqore` mod of Quake4 support, game data directory named `hardqore`. More view in [Quake 4: Hardqore](https://www.moddb.com/mods/quake-4-hardqore).
* Add `ambientLighting` shader, add ambient lighting model(`harm_r_lightingModel` to 4) in DOOM3/Quake4/Prey.
* Add effects color alpha in Quake4.
* Fix `displacement` and `displacementcube` GLSL shader in Quake4. e.g. water in `recomp` map and blood pool in `waste` map.
* Fix weapon model depth hack in player view in Quake4.
* Add player body view in DOOM3/Quake4.
* Add cvar `harm_in_smoothJoystick` to control setup smooth joystick in DOOM3/Quake4/Prey.
* Default enable `Standalone game data directory`.[README.md](README.md)

----------------------------------------------------------------------------------

> 1.1.0harmattan56 (2024-08-23)

* Optimize PBR interaction lighting model in DOOM3/Quake4/Prey.
* Fix environment reflection shader in DOOM3/Quake4/Prey.
* Add ambient lighting model with `harm_r_lightingModel` 0 and remove r_noLight=2 in DOOM3/Quake4/Prey.
* Reduce game crash when change mod/reloadEngine/vid_restart in DOOM3/Quake4/Prey.
* Support switch weapon in DOOM 3(write `bind "YOUR_KEY" "IMPULSE_51"` to your DoomConfig.cfg or autoexec.cfg).
* Add `LibreCoop(RoE)` mod of DOOM3 support, game data directory named `librecoopxp`. More view in [LibreCoop(RoE)](https://www.moddb.com/mods/librecoop-dhewm3-coop).
* Add `Perfected Doom 3` mod of DOOM3 support, game data directory named `perfected`. More view in [Perfected Doom 3](https://www.moddb.com/mods/perfected-doom-3-version-500).
* Add `Perfected Doom 3 : Resurrection of Evil` mod of DOOM3 support, game data directory named `perfected_roe`. More view in [Perfected Doom 3 : Resurrection of Evil](https://www.moddb.com/mods/perfected-doom-3-version-500).

----------------------------------------------------------------------------------

> 1.1.0harmattan55 (2024-08-05) **DOOM3 20th Anniversary Edition**

* Add PBR interaction lighting model([jmarshall23's idtech4_pbr](https://github.com/jmarshall23/idtech4_pbr))(setup cvar harm_r_lightingModel 3) in DOOM3/Quake4/Prey.
* Fix large shake of player view with OpenAL in DOOM3/Quake4/Prey.
* Add command history record manager in launcher.
* Add `/sdcard/Android/data/com.karin.idTech4Amm/files/diii4a` to game data search path(exclude Quake1), add current game data path tips.
* Optimize ETC1 compression texture cache in DOOM3/Quake4/Prey, add ETC2 compression texture support(cvar r_useETC2) in OpenGLES3.0.
* Add launcher theme setting.

----------------------------------------------------------------------------------

> 1.1.0harmattan53 (2024-07-17)

* Fix GZDOOM sound.
* Update screen resolution settings on launcher.
* Add compression textures support with cvar `harm_image_useCompression` for low memory device(e.g. 32bits device, but load slower), and using cvar `harm_image_useCompressionCache` enable caching on DOOM3-BFG.

----------------------------------------------------------------------------------

> 1.1.0harmattan52 (2024-07-11)

* Add soft stencil shadow support(cvar harm_r_stencilShadowSoft) with OpenGLES3.1+ in DOOM3/Quake4/Prey(2006).
* Optimize soft shadow shader with shadow mapping in DOOM3/Quake4/Prey(2006).
* Support r_showSurfaceInfo debug render on multi-threading in DOOM3/Quake4/Prey(2006), need to set cvar `harm_r_renderToolsMultithread` to 1 to enable debug render on multi-threading manually.
* Add GLES3.2 renderer support in Quake2(using +set vid_renderer gles3 for GLES3.2, +set vid_renderer gl1 for GLES1.1).
* Add GZDOOM support on arm64, game data directory named `gzdoom`. More view in [GZDOOM](https://github.com/ZDoom/gzdoom).

----------------------------------------------------------------------------------

> 1.1.0harmattan51 (2024-05-31)

* Add `DOOM 3 BFG`(RBDOOM-3-BFG ver1.4.0) support, game data directory named `doom3bfg/base`. More view in [RBDOOM-3-BFG](https://github.com/RobertBeckebans/RBDOOM-3-BFG) and [DOOM-3-BFG](https://store.steampowered.com/agecheck/app/208200/).
* Add `Quake I`(Darkplaces) support, game data directory named `darkplaces/id1`. More view in [DarkPlaces](https://github.com/DarkPlacesEngine/darkplaces) and [Quake I](https://store.steampowered.com/app/2310/Quake/).
* Fix some shaders error on Mali GPU in The Dark Mod(v2.12).
* Upgrade Quake2(Yamagi Quake II) version.
* Support debug render tools(exclude r_showSurfaceInfo) on multi-threading in DOOM3/Quake4/Prey(2006).
* Support switch lighting disabled in game with r_noLight 0 and 2 in DOOM3/Quake4/Prey(2006).

----------------------------------------------------------------------------------

> 1.1.0harmattan50 (2024-04-30)

* Support new stage rendering of heatHaze shaders(e.g. heat haze distortion of BFG9000's projectile, Rocket Gun's explosion) and colorProcess shader(e.g. blood film on mirror of marscity2).
* Support new shader stage rendering of GLSL shaders in Quake 4(e.g. sniper scope effect of machine gun and bullet hole of machine gun).
* Add control on-screen joystick visible mode in `Control` tab(always show; hidden; only show when pressed).
* Improving Phong/Blinn-Phong light model interaction shader with high-precision.
* Force disable using compression texture in The Dark Mod.
* Game data directories are standalone in Settings: DOOM3 -> doom3/; Quake4 -> quake4/; Prey -> prey/; Quake1 -> quake1/; Quake2 -> quake2/; Quake3 -> quake3/; RTCW -> rtcw/; The Dark Mod -> darkmod/ (always); DOOM3 BFG -> doom3bfg/ (always).

----------------------------------------------------------------------------------

> 1.1.0harmattan39 (2024-04-10)

* Support perforated surface shadow in shadow mapping(cvar `r_forceShadowMapsOnAlphaTestedSurfaces`, default 0).
* Add `LibreCoop` mod of DOOM3 support, game data directory named `librecoop`. More view in [LibreCoop](https://www.moddb.com/mods/librecoop-dhewm3-coop).
* Add `Quake II` support, game data directory named `baseq2`. More view in [Yamagi Quake II](https://github.com/yquake2/yquake2) and [Quake II](https://store.steampowered.com/app/2320/Quake_II/).
* Add `Quake III Arena` support, game data directory named `baseq3`; Add `Quake III Team Arena` support, game data directory named `missionpack`. More view in [ioquake3](https://github.com/ioquake/ioq3) and [Quake III Arena](https://store.steampowered.com/app/2200/Quake_III_Arena/).
* Add `Return to Castle Wolfenstein` support, game data directory named `main`. More view in [iortcw](https://github.com/iortcw/iortcw) and [Return to Castle Wolfenstein](https://www.moddb.com/games/return-to-castle-wolfenstein).
* Add `The Dark Mod` 2.12 support, game data directory named `darkmod`. More view in [The Dark Mod](https://www.thedarkmod.com).
* Add a on-screen button theme.

----------------------------------------------------------------------------------

> 1.1.0harmattan38 (2024-02-05)

* Fixed shadow mapping on non-Adreno GPU.
* Support level loading finished pause(cvar `com_skipLevelLoadPause`) in Quake4.

----------------------------------------------------------------------------------

> 1.1.0harmattan37 (2024-01-06)

* Fixed on-screen buttons initial keycodes.
* On-screen slider button can setup clickable.
* Add dds screenshot support.
* Add cvar `r_scaleMenusTo43` for 4:3 menu.

----------------------------------------------------------------------------------

> 1.1.0harmattan36 (2023-12-31)

* Fixed prelight shadow's shadow mapping.
* Fixed EFX Reverb in Quake4.
* Add translucent stencil shadow support in stencil shadow(bool cvar `harm_r_stencilShadowTranslucent`(default 0); float cvar `harm_r_stencilShadowAlpha` for setting transparency).
* Add float cvar `harm_ui_subtitlesTextScale` control subtitles's text scale in Prey.
* Support cvar `r_brightness`.
* Fixed weapon projectile's scorches decals rendering in Prey(2006).
* Data directory chooser support Android SAF.
* New default on-screen buttons layout.
* Add `Stupid Angry Bot`(a7x) mod of DOOM3 support(need DOOM3: RoE game data), game data directory named `sabot`. More view in [SABot(a7x)](https://www.moddb.com/downloads/sabot-alpha-7x).
* Add `Overthinked DooM^3` mod of DOOM3 support, game data directory named `overthinked`. More view in [Overthinked DooM^3](https://www.moddb.com/mods/overthinked-doom3).
* Add `Fragging Free` mod of DOOM3 support(need DOOM3: RoE game data), game data directory named `fraggingfree`. More view in [Fragging Free](https://www.moddb.com/mods/fragging-free).
* Add `HeXen:Edge of Chaos` mod of DOOM3 support, game data directory named `hexeneoc`. More view in [HeXen:Edge of Chaos](https://www.moddb.com/mods/hexen-edge-of-chaos).

----------------------------------------------------------------------------------

> 1.1.0harmattan35 (2023-10-29)

* Optimize soft shadow with shadow mapping. Add shadow map with depth texture in OpenGLES2.0.
* Add OpenAL(soft) and EFX Reverb support.
* Beam rendering optimization in Prey(2006) by [lvonasek/PreyVR](https://github.com/lvonasek/PreyVR).
* Add subtitle support in Prey(2006).
* Fixed gyroscope in invert-landscape mode.
* Fixed bot head and add bot level control(cvar `harm_si_botLevel`, need extract new `sabot_a9.pk4` resource) in Quake4 MP game.

----------------------------------------------------------------------------------

> 1.1.0harmattan33 (2023-10-01)

* Add shadow mapping soft shadow support(testing, has some incorrect rendering), using `r_useShadowMapping` to change from `shadow mapping` or `stencil shadow`.
* In Quake4, remove Bot FakeClient in multiplayer-game, and add SABot-a9 mod support in multiplayer-game(need extract resource first).
* Fix Setting's tab GUI in Prey2006.
* Add `full-body awareness` mod in Quake4. Set bool cvar `harm_pm_fullBodyAwareness` to 1 enable, and using `harm_pm_fullBodyAwarenessOffset` setup offset(also change to third-person mode), and using `harm_pm_fullBodyAwarenessHeadJoint` setup head joint name(view position).
* Support max FPS limit(cvar `r_maxFps`).
* Support obj/dae static model, and fix png image load.
* Add skip intro support.
* Add simple CVar editor.
* Change OpenGL vertex index size to 4 bytes for large model.
* Add GLES3.0 support, can choose in `Graphics` tab.

----------------------------------------------------------------------------------

> 1.1.0harmattan32 (2023-06-30)

* Add `Chinese`, `Russian`(by [ALord7](https://4pda.ru/forum/index.php?showuser=5043340)) language.
* Move some on-screen settings to `Configure on-screen controls` page.
* Add `full-body awareness` mod in DOOM 3. Set bool cvar `harm_pm_fullBodyAwareness` to 1 enable, and using `harm_pm_fullBodyAwarenessOffset` setup offset(also change to third-person mode).
* Support add external game library in `GameLib` at tab `General`(Testing. Not sure available for all device and Android version because of system security. You can compile own game mod library(armv7/armv8) with DIII4A project and run it using original idTech4A++).
* Support load external game library in `Game working directory`/`fs_game` folder instead of default game library of apk if enabled `Find game library in game data directory`(Testing. Not sure available for all device and Android version because of system security. You can compile own game mod library(armv7/armv8) with DIII4A project, and then named `gameaarch64.so`/`libgameaarch64.so`(arm64 device) or named `gamearm.so`/`libgamearm.so`(arm32 device), and then put it on `Game working directory`/`fs_game` folder, and start game directly with original idTech4A++).
* Support jpg/png image texture file.

----------------------------------------------------------------------------------

> 1.1.0harmattan31 (2023-06-10)

* Add reset all on-screen buttons scale/opacity in tab `CONTROLS`'s `Reset on-screen controls`.
* Add setup all on-screen buttons size in tab `CONTROLS`.
* Add grid assist in tab `CONTROLS`'s `Configure on-screen controls` if setup `On-screen buttons position unit` of settings greater than 0.
* Support unfixed-position joystick and inner dead zone.
* Support custom on-screen button's texture image. If button image file exists in `/sdcard/Android/data/com.karin.idTech4Amm/files/assets` as same file name, will using external image file instead of apk internal image file. Or put button image files as a folder in `/sdcard/Android/data/com.karin.idTech4Amm/files/assets/controls_theme/`, and then select folder name with `Setup on-screen button theme` on `CONTROLS` tab.
* New mouse support implement.

----------------------------------------------------------------------------------

> 1.1.0harmattan30 (2023-05-23)

* Add function key toolbar for soft input method(default disabled, in `Settings`).
* Add joystick release range setting in tab `CONTROLS`. The value is joystick radius's multiple, 0 to disable.
* Fix crash when end intro cinematic in Quake 4.
* Fix delete savegame menu action in Quake 4.

----------------------------------------------------------------------------------

> 1.1.0harmattan29 (2023-05-01)

* Fixup crash in game loading when change app to background.
* Fixup effects with noise and other effects in Quake 4.
* Optimize sky render in Quake 4.
* Remove cvar `harm_g_flashlightOn` in Quake 4.
* Fixup on-screen buttons layer render error on some devices.

----------------------------------------------------------------------------------

> 1.1.0harmattan28 (2023-04-13)

* Add bool cvar `harm_g_mutePlayerFootStep` to control mute player footstep sound(default on) in Quake 4.
* Fix some light's brightness depend on sound amplitude in Quake 4. e.g. in most levels like `airdefense2`, at some dark passages, it should has a repeat-flashing lighting.
* Remove Quake 4 helper dialog when start Quake 4, if want to extract resource files, open `Other` -> `Extract resource` in menu.
* (Bug)In Quake 4, if load some levels has noise with effects on, typed `bse_enabled` to 0, and then typed `bse_enabled` back to 1 in console, noise can resolved.

----------------------------------------------------------------------------------

> 1.1.0harmattan27 (2023-04-05)

* Fixup some line effects in Quake 4. e.g. monster body disappear effect, lines net.
* Fixup radio icon in player HUD right-top corner in Quake 4.
* Fixup dialog interactive in Quake 4. e.g. dialog of creating MP game server.
* Fixup MP game loading GUI and MP game main menu in Quake 4.
* Add integer cvar named `harm_si_autoFillBots` for automatic fill bots after map loaded in MP game in Quake 4(0 to disable). `fillbots` command support append bot num argument.
* Add automatic set bot's team in MP team game, random bot model, and fixup some bot's bugs in Quake 4.
* Add `SABot`'s aas file pak of MP game maps in `Quake 4 helper dialog`.

----------------------------------------------------------------------------------

> 1.1.0harmattan26 (2023-03-25)

* Using SurfaceView for rendering, remove GLSurfaceView(Testing).
* Using DOOM3's Fx/Particle system implement Quake4's BSE incompletely for effects in Quake 4. The effects are bad now. Using set `bse_enabled` to 0 for disable effects.
* Remove my cvar `harm_g_alwaysRun`, so set original `in_alwaysRun` to 1 for run in Quake 4.
* Add simple beam model render in Prey(2006).
* Optimize skybox render in Prey(2006) by [lvonasek/PreyVR](https://github.com/lvonasek/PreyVR).

----------------------------------------------------------------------------------

> 1.1.0harmattan25 (2023-02-22)

* Sound with OpenSLES support(Testing).
* Add backup/restore preferences support.
* Add menu music playing in Prey(2006).
* Add map loading music playing in Prey(2006).
* Add entity visible/invisible in spirit walk mode in Prey(2006), e.g. spirit bridge.
* Optimize portal render with view distance in Prey(2006).

----------------------------------------------------------------------------------

> 1.1.0harmattan23 (2023-02-16)

* Multi-threading support(Testing, NO `r_multithread` cvar, DO NOT SUPPORT TO CHANGE WITH MULTI-THREAD AND SINGLE-THREAD IN GAME AT PRESENT!!! ONLY CAN SETTING IN LAUNCHER!!!), using [d3es-multithread](https://github.com/emileb/d3es-multithread).
* Fixup portal/skybox view in Prey(2006).
* Fixup intro sound playing when start new game in Prey(2006) by [lvonasek/PreyVR](https://github.com/lvonasek/PreyVR).
* Fixup player can not through first wall with spirit walk mode in `game/spindlea` beginning in Prey(2006).
* Fixup render Tommy's original body when in spirit walk mode in Prey(2006).
* Do not render on-screen buttons when game is loading.

----------------------------------------------------------------------------------

> 1.1.0harmattan22 (2023-01-10)

* Support screen top edges with fullscreen.
* Add bad skybox render in Prey(2006)(Fixed in version 23).
* Add bad portal render in Prey(2006)(Fixed in version 23).
* Add `deathwalk` map append support in Prey(2006), but now has a bug so don't save game when player in `deathwalk` status.
* If not sound when start new game and load `roadhouse` map, try to press `ESC` key back to main menu and then press `ESC` key to back game, then sound can be played(Fixed in version 23).

----------------------------------------------------------------------------------

> 1.1.0harmattan21 (2022-12-10)

* Prey(2006) for DOOM3 support, game data folder named `preybase`. All levels clear, but have some bugs.
* Add setup On-screen buttons position unit when config controls layout.
* Android Target SDK level back to 28(Android 9), for avoid `Scoped-Storage` on Android 10+.

----------------------------------------------------------------------------------

> 1.1.0harmattan20 (2022-11-18)

* Add default font for somewhere missing text in Quake 4, using cvar `harm_gui_defaultFont` to control, default is `chain`.
* Implement show surface/hide surface for fixup entity render incorrect in Quake 4, e.g. AI's weapons, weapons in player view and Makron in boss level.

----------------------------------------------------------------------------------

> 1.1.0harmattan19 (2022-11-16)

* Fixup middle bridge door GUI not interactive of level `game/tram1` in Quake 4.
* Fixup elevator 1 with a monster GUI not interactive of level `game/process2` in Quake 4.

----------------------------------------------------------------------------------

> 1.1.0harmattan18 (2022-11-11)

* Implement some debug render functions.
* Add player focus GUI bracket and interactive text on HUD in Quake 4.
* Automatic generating AAS file for bot of Multiplayer-Game maps is not need enable net_allowCheats when set cvar `harm_g_autoGenAASFileInMPGame` to 1 in Quake 4.
* Fixed restart menu action in Quake 4.
* Fixed a memory bug that can cause crash in Quake 4.

----------------------------------------------------------------------------------

> 1.1.0harmattan17 (2022-10-29)

* Support Quake 4 format fonts. Other language patches will work. D3-format fonts do not need to extract no longer.
* Solution of some GUIs can not interactive in Quake 4, you can try `quicksave`, and then `quickload`, the GUI can interactive. E.g. 1. A door's control GUI on bridge of level `game/tram1`, 2. A elevator's control GUI with a monster of `game/process2`(Fixed in version 19).

----------------------------------------------------------------------------------

> 1.1.0harmattan16 (2022-10-22)

* Add automatic load `QuickSave` when start game.
* Add control Quake 4 helper dialog visible when start Quake 4 in Settings, and add `Extract Quake 4 resource` in `Other` menu.
* Add setup all on-screen button opacity.
* Support checking for update from GitHub.
* Fixup some Quake 4 bugs:
> 1. Fixup collision, e.g. trigger, vehicle, AI, elevator, health-station. So fixed block on last elevator in level `game/mcc_landing` and fixed incorrect collision cause killing player on elevator in `game/process1 first` and `game/process1 second` and fixed block when player jumping form vehicle in `game/convoy1`. And cvar `harm_g_useSimpleTriggerClip` is removed.
> 2. Fixup game level load fatal error and crash in `game/mcc_1` and `game/tram1b`. So all levels have not fatal error now.

----------------------------------------------------------------------------------

> 1.1.0harmattan15 (2022-10-15)

* Add gyroscope control support.
* Add reset onscreen button layout with fullscreen.
* If running Quake 4 crash on arm32 device, trying to check `Use ETC1 compression` or `Disable lighting` for decreasing memory usage.
* Fixup some Quake 4 bugs:
> 1. Fixup start new game in main menu, now start new game is work.
> 2. Fixup loading zombie material in level `game/waste`.
> 3. Fixup AI `Singer` can not move when opening the door in level `game/building_b`.
> 4. Fixup jump down on broken floor in level `game/putra`.
> 5. Fixup player model choice and view in `Settings` menu in Multiplayer game.
> 6. Add bool cvar `harm_g_flashlightOn` for controlling gun-lighting is open/close initial, default is 1(open).
> 7. Add bool cvar `harm_g_vehicleWalkerMoveNormalize` for re-normalize `vehicle walker` movement if enable `Smooth joystick` in launcher, default is 1(re-normalize), it can fix up move left-right.

----------------------------------------------------------------------------------

> 1.1.0harmattan13 (2022-10-23)

* Fixup Strogg health station GUI interactive in `Quake 4`.
* Fixup skip cinematic in `Quake 4`.
* If `harm_g_alwaysRun` is 1, hold `Walk` key to walk in `Quake 4`(Removed in version 26, using original `in_alwaysRun`).
* Fixup level map script fatal error or bug in `Quake 4`(All maps have not fatal errors no longer, but have some bugs yet.).
> 1. `game/mcc_landing`: Player collision error on last elevator. You can jump before elevator ending or using `noclip`(Fixed in version 16).
> 2. `game/mcc_1`: Loading crash after last level ending. Using `map game/mcc_1` to reload(Fixed in version 16).
> 3. `game/convoy1`: State error is not care no longer and ignore. But sometimes has player collision error when jumping form vehicle, using `noclip`(Fixed in version 16).
> 4. `game/putra`: Script fatal error has fixed. But can not down on broken floor, using `noclip`(Fixed in version 15).
> 5. `game/waste`: Script fatal error has fixed.
> 6. `game/process1 first`: Last elevator has ins collision cause killing player(Fixed in version 16). Using `god`. If tower's elevator GUI not work, using `teleport tgr_endlevel` to next level directly.
> 7. `game/process1 second`: Second elevator has incorrect collision cause killing player(same as `game/process1 first` level). Using `god`(Fixed in version 16).
> 8. `game/tram_1b`: Loading crash after last level ending. Using `map game/tram_1b` to reload(Fixed in version 16).
> 9. `game/core1`: Fixup first elevator platform not go up.
> 10. `game/core2`: Fixup entity rotation.

----------------------------------------------------------------------------------

> 1.1.0harmattan12 (2022-07-19)

 * `Quake 4` in DOOM3 engine support. Also see `https://github.com/jmarshall23/Quake4Doom`. Now can play most levels, but some levels has error.
 * Quake 4 game data folder named `q4base`, also see `https://store.steampowered.com/app/2210/Quake_4/`.
 * Fix `Rivensin` and `Hardcorps` mod load game from save game.
 * Add console command history record.
 * On-screen buttons layer's resolution always same to device screen.
 * Add volume key map config(Enable `Map volume keys` to show it).

----------------------------------------------------------------------------------

> 1.1.0harmattan11 (2022-06-30)

 * Add `Hardcorps` mod library support, game path name is `hardcorps`, if play the mod, first suggest to close `Smooth joystick` in `Controls` tab panel, more view in `https://www.moddb.com/mods/hardcorps`.
 * In `Rivensin` mod, add bool Cvar `harm_pm_doubleJump` to enable double-jump(From `hardcorps` mod source code, default disabled).
 * In `Rivensin` mod, add bool Cvar `harm_pm_autoForceThirdPerson` for auto set `pm_thirdPerson` to 1 after level load end when play original DOOM3 maps(Default disabled).
 * In `Rivensin` mod, add float Cvar `harm_pm_preferCrouchViewHeight` for view poking out some tunnel's ceil when crouch(Default 0 means disabled, and also can set `pm_crouchviewheight` to a smaller value).
 * Add on-screen button config page, and reset some on-screen button keymap to DOOM3 default key.
 * Add menu `Special Cvar list` in `Other` menu for list all new special `Cvar`.

----------------------------------------------------------------------------------

> 2022-06-23 Update 1.1.0harmattan10

* Add `Rivensin` mod library support, game path name is `rivensin`, more view in `https://www.moddb.com/mods/ruiner`.
* The `Rivensin` game library support load DOOM3 base game map. But first must add include original DOOM3 all map script into `doom_main.script` of `Rivensin` mod file.
* Add weapon panel keys configure.
* Fix file access permission grant on Android 10(Sorry for I have not Android 10/11+ device to testing).

----------------------------------------------------------------------------------

> 2022-06-15 Update 1.1.0harmattan9

* Fix file access permission grant on Android 11+.
* Add Android 4.x apk package v1 sign.

----------------------------------------------------------------------------------

> 2022-05-19 Update 1.1.0harmattan8

DIII4A++_harmattan.1.1.0.8.apk: include armv8-64 and armv7 32 neon library.
DIII4A++_harmattan.1.1.0.8_only_armv7a.apk: only include armv7 32 neon library.

* Compile armv8-a 64 bits library, and set FPU neon is default on armv7-a, and do not compile old armv5e library and armv7-a vfp.
* Fix input event when modal MessageBox is visible in game.
* Add cURL support for downloading in multiplayer game.
* Add weapon on-screen button panel.

----------------------------------------------------------------------------------

> 2022-05-05 1.1.0harmattan7

Update:
* Fix shadow clipped.
* Fix sky box.
* Fix fog and blend light.
* Fix glass reflection.
* Add `texgen` shader for like `D3XP` hell level sky.
* Fix translucent object. i.e. window glass, translucent Demon in `Classic DOOM` mod.
* Fix dynamic texture interaction. i.e. rotating fans.
* Fix `Berserk`, `Grabber`, `Helltime` vision effect(First set cvar `harm_g_skipBerserkVision`, `harm_g_skipWarpVision` and `harm_g_skipHelltimeVision` to 0).
* Fix screen capture image when quick save game or mission tips.
* Fix machine gun's ammo panel.
* Add light model setting with `Phong` and `Blinn-Phong` when render interaction shader pass(string cvar `harm_r_lightingModel`).
* Add specular exponent setting in light model(float cvar `harm_r_specularExponent`).
* Default using program internal OpenGL shader.
* Reset extras virtual button size, and add Console(~) key.
* Add `Back` key function setting, add 3-Click to exit.
* Add cvar `harm_r_shadowCarmackInverse` to change general Z-Fail stencil shadow or `Carmack-Inverse` Z-Fail stencil shadow.
* DIII4A build on Android Studio now.

----------------------------------------------------------------------------------

> 2020-08-25 1.1.0harmattan6

* Fix video playing - 1.1.0harmattan6.
* Choose game library when load other game mod, more view in `Help` menu - 1.1.0harmattan6.
* Fix game audio sound playing(Testing) - 1.1.0harmattan5.
* Add launcher orientation setting on `CONTROLS` tab - 1.1.0harmattan5.

----------------------------------------------------------------------------------

> 2020-08-17 1.1.0harmattan3

* Uncheck 4 checkboxs, default value is 0(disabled).
* Hide software keyboard when open launcher activity.
* Check `WRITE_EXTERNAL_STORAGE` permission when start game or edit config file.
* Add game data directory chooser.
* Add `Save settings` menu if you only change settings but don't want to start game.
* UI editor can hide navigation bar if checked `Hide navigation bar`(the setting must be saved before do it).
* Add `Help` menu.

----------------------------------------------------------------------------------

> 2020-08-16 1.1.0harmattan2

Notification:
* If you have installed other version apk(package name is `com.n0n3m4.diii4a`) of other sources, you first to uninstall the old version apk package named `com.n0n3m4.diii4a`, after install this new version apk. Because the apk package is same `com.n0n3m4.diii4a`, but certificate is different.
* If app running crash(white screen), first make sure to allow `WRITE_EXTERNAL_STORAGE` permission, alter please uncheck 4th checkbox named `Use ETC1(or RGBA4444) cache` or clear ETC1 texture cache file manual on resource folder(exam. /sdcard/diii4a/<base/d3xp/d3le/cdoom/or...>/dds).
* `Clear vertex buffer` suggest to select 3rd or 2nd for clear vertex buffer every frame! If you select 1st, it will be same as original apk, maybe flash and crash with out of graphics memory! More view in game, on DOOM3 console, cvar named `harm_r_clearVertexBuffer`.
* TODO: `Classic DOOM` some trigger can not interact, exam last door of `E1M1`. I don't know what reason. But you can toggle `noclip` with console or shortcut key to through it.

----------------------------------------------------------------------------------

> 2020-08-16 1.1.0harmattan1

* Compile `DOOM3:RoE` game library named `libd3xp`, game path name is `d3xp`, more view in `https://store.steampowered.com/app/9070/DOOM_3_Resurrection_of_Evil/`.
* Compile `Classic DOOM3` game library named `libcdoom`, game path name is `cdoom`, more view in `https://www.moddb.com/mods/classic-doom-3`.
* Compile `DOOM3-BFG:The lost mission` game library named `libd3le`, game path name is `d3le`, need `d3xp` resources(+set fs_game_base d3xp), more view in `https://www.moddb.com/mods/the-lost-mission`(now fix stack overflow when load model `models/mapobjects/hell/hellintro.lwo` of level `game/le_hell` map on Android).
	* Clear vertex buffer for graphics memory overflow(integer cvar `harm_r_clearVertexBuffer`).
	* Skip visual vision for `Berserk Powerup` on `DOOM3`(bool cvar `harm_g_skipBerserkVision`).
	* Skip visual vision for `Grabber` on `D3 RoE`(bool cvar `harm_g_skipWarpVision`).
	* Skip visual vision for `Helltime Powerup` on `D3 RoE`(bool cvar `harm_g_skipHelltimeVision`).
* Add support to run on background.
* Add support to hide navigation bar.
* Add RGBA4444 16-bits color.
* Add config file editor.
	
----------------------------------------------------------------------------------
