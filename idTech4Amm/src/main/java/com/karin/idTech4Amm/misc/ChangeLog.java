package com.karin.idTech4Amm.misc;

import com.karin.idTech4Amm.sys.Constants;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class ChangeLog
{
    public String       date;
    public int          release;
    public List<String> logs;

    public ChangeLog Date(String date)
    {
        this.date = date;
        return this;
    }

    public ChangeLog Release(int release)
    {
        this.release = release;
        return this;
    }

    public ChangeLog Log(String...args)
    {
        if(logs == null)
            logs = new ArrayList<>();
        Collections.addAll(logs, args);
        return this;
    }

    @Override
    public String toString()
    {
        return GenString(TextHelper.GetDialogMessageEndl());
    }

    public String GenString(String endl)
    {
        StringBuilder sb = new StringBuilder();
        sb.append("------- ").append(date).append(" (R").append(release).append(") -------");
        sb.append(endl);
        if(logs != null && !logs.isEmpty())
        {
            for(String str : logs)
            {
                if(str != null)
                    sb.append(TextHelper.FormatDialogMessageSpace("  * ")).append(str);
                sb.append(endl);
            }
        }
        return sb.toString();
    }

    public static ChangeLog Create(String date, int release, String...args)
    {
        ChangeLog cl = new ChangeLog();
        cl.Date(date)
                .Release(release)
                .Log(args)
        ;
        return cl;
    }

    public static ChangeLog[] GetChangeLogs()
    {
        final ChangeLog[] CHANGES = {
                ChangeLog.Create(Constants.CONST_RELEASE, Constants.CONST_UPDATE_RELEASE, Constants.CONST_CHANGES()),

                ChangeLog.Create("2025-03-11", 62,
                        "Add `OpenJK` support, `" + TextHelper.GenLinkText("https://store.steampowered.com/app/6020/STAR_WARS_Jedi_Knight__Jedi_Academy/", "STAR WARS™ Jedi Knight - Jedi Academy™") + "` game standalone directory named `openja`, `" + TextHelper.GenLinkText("https://store.steampowered.com/app/6030/STAR_WARS_Jedi_Knight_II__Jedi_Outcast/", "STAR WARS™ Jedi Knight II - Jedi Outcast™") + "` game standalone directory named `openjo`. More view in `" + TextHelper.GenLinkText("https://github.com/JACoders/OpenJK", "OpenJK") + "`.",
                        "Update version to 5.1 on RealRTCW, support Survival mode. And version 5.0 is keeping until version 5.1 is stable.",
                        "Fix audio playing when disable OpenAL on Quake3/ETW/RTCW/RealRTCW.",
                        "Add Vulkan renderer backend on Quake2.",
                        "Fix create render context on GZDOOM, it will use OpenGL ES if Vulkan initialization fail.",
                        "Add `Open menu` button at end of `General` tab on launcher."
                ),

                ChangeLog.Create("2025-02-27", 61,
                        "Add `FTEQW` support, game standalone directory named `fteqw`, support `" + TextHelper.GenLinkText("https://store.steampowered.com/app/9060/HeXen_II/", "HeXen II") + "`, `Half-Life` " + TextHelper.GenLinkText("https://github.com/eukara/freehl", "FreeHL") + ", `Counter Striker 1.5` " + TextHelper.GenLinkText("https://github.com/eukara/freecs", "FreeCS") + ". More view in `" + TextHelper.GenLinkText("https://www.fteqw.org", "FTEQW") + "`.",
                        "DOOM 3 BFG add Vulkan renderer backend.",
                        "Don't package source code to apk since version 61."
                ),

                ChangeLog.Create("2025-01-16", 60,
                        "Support setup max game console height percentage(0 or 100 means not limit) on launcher `General` tab.",
                        "Update GZDOOM version to 4.14.0.",
                        "GZDOOM add Vulkan and OpenGL renderer backend.",
                        "Update Wolfenstein: Enemy Territory(ET: Legacy) version to 2.83.1.",
                        "Update Quake 1(Darkplaces) version.",
                        "Update Quake 2(yquake2) version.",
                        "Add use multisamples config in game."
                ),

                ChangeLog.Create("2024-11-20", 59,
                        "Support `Omni-Bot` in Wolfenstein: Enemy Territory.",
                        "Fix rendering on Mali GPU in DOOM 3-BFG.",
                        "Fix rendering on Mali GPU in The Dark Mod.",
                        "Fix stencil shadow with `cg_shadows` = 2 in Wolfenstein: Enemy Territory.",
                        "Support choose a mod directory in GZDOOM.",
                        "Add some new features options on launcher in Wolfenstein: Enemy Territory, RealRTCW, DOOM3-BFG, Quake 2, GZDOOM, The Dark Mod.",
                        "Add use high precision float on GLSL shaders(cvar `harm_r_useHighPrecision`) in DOOM 3/Quake 4/Prey.",
                        "Add 5 onscreen buttons.",
                        "Add `Phobos(for Dhewm3)` mod of DOOM3 support, game data directory named `tfphobos`(d3xp and dhewm3 compatibility patch required). More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/phobos", "Doom 3: Phobos") + "` and .`" + TextHelper.GenLinkText("https://www.moddb.com/games/doom-iii/addons/doom-3-phobos-dhewm3-compatibility-patch", "Doom 3: Phobos - dhewm3 compatibility patch") + "`."
                ),

                ChangeLog.Create("2024-10-01", 58,
                        "Add `RealRTCW`(ver 5.0) support, game standalone directory named `realrtcw`, game data directory named `Main`. More view in `" + TextHelper.GenLinkText("https://github.com/wolfetplayer/RealRTCW", "RealRTCW") + "`.",
                        "Fix light bar indicator of player's HUD by `darkness257` on The Dark Mod, now setup `tdm_lg_weak` to 1 automatically. More view in `" + TextHelper.GenLinkText("https://github.com/glKarin/com.n0n3m4.diii4a/issues/244", "The Darkmod light bar indicator bug") + "`.",
                        "Support create desktop shortcut for games or current command on `Option` menu of launcher.",
                        "Fix stencil shadow with `cg_shadows` = 2 on Quake 3.",
                        "Improve stencil shadow with `cg_shadows` = 2 on RealRTCW."
                ),

                ChangeLog.Create("2024-10-01", 57,
                        "Add `Wolfenstein: Enemy Territory` support, game standalone directory named `etw`, game data directory named `etmain` and `legacy`. More view in `" + TextHelper.GenLinkText("https://www.etlegacy.com", "ET: Legacy") + "`.",
                        "Add `Quake 4: Hardqore` mod of Quake4 support, game data directory named `hardqore`. More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/quake-4-hardqore", "Quake 4: Hardqore") + "`.",
                        "Add `ambientLighting` shader, add ambient lighting model(`harm_r_lightingModel` to 4) in DOOM3/Quake4/Prey.",
                        "Add effects color alpha in Quake4.",
                        "Fix `displacement` and `displacementcube` GLSL shader in Quake4. e.g. water in `recomp` map and blood pool in `waste` map.",
                        "Fix weapon model depth hack in player view in Quake4.",
                        "Add player body view in DOOM3/Quake4.",
                        "Add cvar `harm_in_smoothJoystick` to control setup smooth joystick in DOOM3/Quake4/Prey.",
                        "Default enable `Standalone game data directory`."
                ),

                ChangeLog.Create("2024-08-23", 56,
                        "Optimize PBR interaction lighting model in DOOM3/Quake4/Prey.",
                        "Fix environment reflection shader in DOOM3/Quake4/Prey.",
                        "Add no-lighting with `harm_r_lightingModel` 0 and remove r_noLight=2 in DOOM3/Quake4/Prey.",
                        "Reduce game crash when change mod/reloadEngine/vid_restart in DOOM3/Quake4/Prey.",
                        "Support switch weapon in DOOM 3(write `bind \"YOUR_KEY\" \"IMPULSE_51\"` to your DoomConfig.cfg or autoexec.cfg).",
                        "Add `LibreCoop(RoE)` mod of DOOM3 support, game data directory named `librecoopxp`(d3xp required). More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/librecoop-dhewm3-coop", "LibreCoop") + "`.",
                        "Add `Perfected Doom 3` mod of DOOM3 support, game data directory named `perfected`. More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/perfected-doom-3-version-500", "Perfected Doom 3") + "`.",
                        "Add `Perfected Doom 3 : Resurrection of Evil` mod of DOOM3 support, game data directory named `perfected_roe`(d3xp required). More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/perfected-doom-3-version-500", "Perfected Doom 3 : Resurrection of Evil") + "`."
                ),

                ChangeLog.Create("2024-08-05", 55,
                        "Add PBR interaction lighting model(" + TextHelper.GenLinkText("https://github.com/jmarshall23/idtech4_pbr", "jmarshall23's idtech4_pbr") + ")(setup cvar harm_r_lightingModel 3) in DOOM3/Quake4/Prey.",
                        "Fix large shake of player view with OpenAL in DOOM3/Quake4/Prey.",
                        "Add command history record manager in launcher.",
                        "Add `/sdcard/Android/data/com.karin.idTech4Amm/files/diii4a` to game data search path(exclude Quake1), add current game data path tips.",
                        "Optimize ETC1 compression texture cache in DOOM3/Quake4/Prey, add ETC2 compression texture support(cvar r_useETC2) in OpenGLES3.0.",
                        "Add launcher theme setting."
                ),

                ChangeLog.Create("2024-07-17", 53,
                        "Fix GZDOOM sound.",
                        "Update screen resolution settings on launcher.",
                        "Add compression textures support with cvar `harm_image_useCompression` for low memory device(e.g. 32bits device, but load slower), and using cvar `harm_image_useCompressionCache` enable caching on DOOM3-BFG."
                ),

                ChangeLog.Create("2024-07-11", 52,
                        "Add soft stencil shadow support(cvar `harm_r_stencilShadowSoft`) with OpenGLES3.1+ in DOOM3/Quake4/Prey(2006).",
                        "Optimize soft shadow shader with shadow mapping in DOOM3/Quake4/Prey(2006).",
                        "Support r_showSurfaceInfo debug render on multi-threading in DOOM3/Quake4/Prey(2006), need to set cvar `harm_r_renderToolsMultithread` to 1 to enable debug render on multi-threading manually.",
                        "Add GLES3.2 renderer support in Quake2(using +set vid_renderer gles3 for GLES3.2, +set vid_renderer gl1 for GLES1.1).",
                        "Add GZDOOM support on arm64, game data directory named `gzdoom`. More view in `" + TextHelper.GenLinkText("https://github.com/ZDoom/gzdoom", "GZDOOM") + "`."
                ),

                ChangeLog.Create("2024-05-31", 51,
                        "Add `DOOM 3 BFG`(RBDOOM-3-BFG ver1.4.0) support, game data directory named `doom3bfg/base`. More view in `" + TextHelper.GenLinkText("https://github.com/RobertBeckebans/RBDOOM-3-BFG", "RBDOOM-3-BFG") + "` and `" + TextHelper.GenLinkText("https://store.steampowered.com/agecheck/app/208200/", "DOOM-3-BFG") + "`.",
                        "Add `Quake I`(Darkplaces) support, game data directory named `darkplaces/id1`. More view in `" + TextHelper.GenLinkText("https://github.com/DarkPlacesEngine/darkplaces", "DarkPlaces") + "` and `" + TextHelper.GenLinkText("https://store.steampowered.com/app/2310/Quake/", "Quake I") + "`.",
                        "Fix some shaders error on Mali GPU in The Dark Mod(v 2.12).",
                        "Upgrade Quake2(Yamagi Quake II) version.",
                        "Support debug render tools(exclude r_showSurfaceInfo) on multi-threading in DOOM3/Quake4/Prey(2006).",
                        "Support switch lighting disabled in game with r_noLight 0 and 2 in DOOM3/Quake4/Prey(2006)."
                ),

                ChangeLog.Create("2024-04-30", 50,
                        "Support new stage rendering of heatHaze shaders(e.g. heat haze distortion of BFG9000's projectile, Rocket Gun's explosion) and colorProcess shader(e.g. blood film on mirror of marscity2).",
                        "Support new shader stage rendering of GLSL shaders in Quake 4(e.g. sniper scope effect of machine gun and bullet hole of machine gun).",
                        "Add control on-screen joystick visible mode in `Control` tab(always show; hidden; only show when pressed).",
                        "Improving Phong/Blinn-Phong light model interaction shader with high-precision.",
                        "Force disable using compression texture in The Dark Mod.",
                        "Game data directories are standalone in Settings: DOOM3 -> doom3/; Quake4 -> quake4/; Prey -> prey/; Quake1 -> quake1/; Quake2 -> quake2/; Quake3 -> quake3/; RTCW -> rtcw/; The Dark Mod -> darkmod/ (always); DOOM3 BFG -> doom3bfg/ (always)."
                ),

                ChangeLog.Create("2024-04-10", 39,
                        "Support perforated surface shadow in shadow mapping(cvar `r_forceShadowMapsOnAlphaTestedSurfaces`, default 0).",
                        "Add `LibreCoop` mod of DOOM3 support, game data directory named `librecoop`. More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/librecoop-dhewm3-coop", "LibreCoop") + "`.",
                        "Add `Quake II` support, game data directory named `baseq2`. More view in `" + TextHelper.GenLinkText("https://github.com/yquake2/yquake2", "Yamagi Quake II") + "` and `" + TextHelper.GenLinkText("https://store.steampowered.com/app/2320/Quake_II/", "Quake II") + "`.",
                        "Add `Quake III Arena` support, game data directory named `baseq3`; Add `Quake III Team Arena` support, game data directory named `missionpack`. More view in `" + TextHelper.GenLinkText("https://github.com/ioquake/ioq3", "ioquake3") + "` and `" + TextHelper.GenLinkText("https://store.steampowered.com/app/2200/Quake_III_Arena/", "Quake III Arena") + "`.",
                        "Add `Return to Castle Wolfenstein` support, game data directory named `main`. More view in `" + TextHelper.GenLinkText("https://github.com/iortcw/iortcw", "iortcw") + "` and `" + TextHelper.GenLinkText("https://www.moddb.com/games/return-to-castle-wolfenstein", "Return to Castle Wolfenstein") + "`.",
                        "Add `The Dark Mod` support, game data directory named `darkmod`. More view in `" + TextHelper.GenLinkText("https://www.thedarkmod.com", "The Dark Mod") + "`.",
                        "Add a on-screen button theme."
                ),

                ChangeLog.Create("2024-02-05", 38,
                        "Fixed shadow mapping on non-Adreno GPU.",
                        "Support level loading finished pause(cvar `com_skipLevelLoadPause`) in Quake4."
                ),

                ChangeLog.Create("2024-01-06", 37,
                        "Fixed on-screen buttons initial keycodes.",
                        "On-screen slider button can setup clickable.",
                        "Add dds screenshot support.",
                        "Add cvar `r_scaleMenusTo43` for 4:3 menu."
                ),

                ChangeLog.Create("2023-12-31", 36,
                        "Fixed prelight shadow's shadow mapping.",
                        "Fixed EFX Reverb in Quake4.",
                        "Add translucent stencil shadow support in stencil shadow(bool cvar `harm_r_stencilShadowTranslucent`(default 0); float cvar `harm_r_stencilShadowAlpha` for setting transparency).",
                        "Add float cvar `harm_ui_subtitlesTextScale` control subtitles's text scale in Prey.",
                        "Support cvar `r_brightness`.",
                        "Fixed weapon projectile's scorches decals rendering in Prey(2006).",
                        "Data directory chooser support Android SAF.",
                        "New default on-screen buttons layout.",
                        "Add `Stupid Angry Bot`(a7x) mod of DOOM3 support(need DOOM3: RoE game data), game data directory named `sabot`. More view in`" + TextHelper.GenLinkText("https://www.moddb.com/downloads/sabot-alpha-7x", "SABot(a7x)") + "`.",
                        "Add `Overthinked DooM^3` mod of DOOM3 support, game data directory named `overthinked`. More view in`" + TextHelper.GenLinkText("https://www.moddb.com/mods/overthinked-doom3", "Overthinked DooM^3") + "`.",
                        "Add `Fragging Free` mod of DOOM3 support(need DOOM3: RoE game data), game data directory named `fraggingfree`. More view in`" + TextHelper.GenLinkText("https://www.moddb.com/mods/fragging-free", "Fragging Free") + "`.",
                        "Add `HeXen:Edge of Chaos` mod of DOOM3 support, game data directory named `hexeneoc`. More view in`" + TextHelper.GenLinkText("https://www.moddb.com/mods/hexen-edge-of-chaos", "HexenEOC") + "`."
                ),

                ChangeLog.Create("2023-10-29", 35,
                        "Optimize soft shadow with shadow mapping. Add shadow map with depth texture in OpenGLES2.0.",
                        "Add OpenAL(soft) and EFX Reverb support.",
                        "Beam rendering optimization in Prey(2006) by `" + TextHelper.GenLinkText("https://github.com/lvonasek/PreyVR", "lvonasek/PreyVR") + "`.",
                        "Add subtitle support in Prey(2006).",
                        "Fixed gyroscope in invert-landscape mode.",
                        "Fixed bot head and add bot level control(cvar `harm_si_botLevel`, need extract new `sabot_a9.pk4` resource) in Quake4 MP game."
                ),

                ChangeLog.Create("2023-10-01", 33,
                        "Add shadow mapping soft shadow support(testing, has some incorrect rendering), using `r_useShadowMapping` to change from `shadow mapping` or `stencil shadow`.",
                        "In Quake4, remove Bot FakeClient in multiplayer-game, and add SABot-a9 mod support in multiplayer-game(need extract resource first).",
                        "Fix Setting's tab GUI in Prey2006.",
                        "Add `full-body awareness` mod in Quake4. Set bool cvar `harm_pm_fullBodyAwareness` to 1 enable, and using `harm_pm_fullBodyAwarenessOffset` setup offset(also change to third-person mode), and using `harm_pm_fullBodyAwarenessHeadJoint` setup head joint name(view position).",
                        "Support max FPS limit(cvar `r_maxFps`).",
                        "Support obj/dae static model, and fix png image load.",
                        "Add skip intro support.",
                        "Add simple CVar editor.",
                        "Change OpenGL vertex index size to 4 bytes for large model.",
                        "Add GLES3.0 support, can choose in `Graphics` tab."
                ),

                ChangeLog.Create("2023-06-30", 32,
                        "Add `Chinese`, `Russian`(by " + TextHelper.GenLinkText("https://4pda.ru/forum/index.php?showuser=5043340", "ALord7") + ") language.",
                        "Move some on-screen settings to `Configure on-screen controls` page.",
                        "Add `full-body awareness` mod in DOOM 3. Set bool cvar `harm_pm_fullBodyAwareness` to 1 enable, and using `harm_pm_fullBodyAwarenessOffset` setup offset(also change to third-person mode).",
                        "Support add external game library in `GameLib` at tab `General`(Testing. Not sure available for all device and Android version because of system security. You can compile own game mod library(armv7/armv8) with DIII4A project and run it using original idTech4A++).",
                        "Support load external game library in `Game working directory`/`fs_game` folder instead of default game library of apk if enabled `Find game library in game data directory`(Testing. Not sure available for all device and Android version because of system security. You can compile own game mod library(armv7/armv8) with DIII4A project, and then named `gameaarch64.so`/`libgameaarch64.so`(arm64 device) or named `gamearm.so`/`libgamearm.so`(arm32 device), and then put it on `Game working directory`/`fs_game` folder, and start game directly with original idTech4A++).",
                        "Support jpg/png image texture file."
                ),

                ChangeLog.Create("2023-06-10", 31,
                        "Add reset all on-screen buttons scale/opacity in tab `CONTROLS`'s `Reset on-screen controls`.",
                        "Add setup all on-screen buttons size in tab `CONTROLS`.",
                        "Add grid assist in tab `CONTROLS`'s `Configure on-screen controls` if setup `On-screen buttons position unit` of settings greater than 0.",
                        "Support unfixed-position joystick and inner dead zone.",
                        "Support custom on-screen button's texture image. If button image file exists in `/sdcard/Android/data/" + Constants.CONST_PACKAGE + "/files/assets` as same file name, will using external image file instead of apk internal image file. Or put button image files as a folder in `/sdcard/Android/data/" + Constants.CONST_PACKAGE + "/files/assets/controls_theme/`, and then select folder name with `Setup on-screen button theme` on `CONTROLS` tab.",
                        "New mouse support implement."
                ),

                ChangeLog.Create("2023-05-25", 30,
                        "Add function key toolbar for soft input method(default disabled, in Settings).",
                        "Add joystick release range setting in tab `CONTROLS`. The value is joystick radius's multiple, 0 to disable.",
                        "Fix crash when end intro cinematic in Quake 4.",
                        "Fix delete savegame menu action in Quake 4."
                ),

                ChangeLog.Create("2023-05-01", 29,
                        "Fixup crash in game loading when change app to background.",
                        "Fixup effects with noise in Quake 4.",
                        "Optimize sky render in Quake 4.",
                        "Remove cvar `harm_g_flashlightOn` in Quake 4.",
                        "Fixup on-screen buttons layer render error on some devices."
                ),

                ChangeLog.Create("2023-04-13", 28,
                        "Add bool cvar `harm_g_mutePlayerFootStep` to control mute player footstep sound(default on) in Quake 4.",
                        "Fix some light's brightness depend on sound amplitude in Quake 4. e.g. in most levels like `airdefense2`, at some dark passages, it should has a repeat-flashing lighting.",
                        "Remove Quake 4 helper dialog when start Quake 4, if want to extract resource files, open `Other` -> `Extract resource` in menu.",
                        "(Bug)In Quake 4, if load some levels has noise with effects on, typed `bse_enable` to 0, and then typed `bse_enable` back to 1 in console, noise can resolved."
                ),

                ChangeLog.Create("2023-04-05", 27,
                        "Fixup some line effects in Quake 4. e.g. monster body disappear effect, lines net.",
                        "Fixup radio icon in player HUD right-top corner in Quake 4.",
                        "Fixup dialog interactive in Quake 4. e.g. dialog of creating MP game server.",
                        "Fixup MP game loading GUI and MP game main menu in Quake 4.",
                        "Add integer cvar named `harm_si_autoFillBots` for automatic fill bots after map loaded in MP game in Quake 4(0 to disable). `fillbots` command support append bot num argument.",
                        "Add automatic set bot's team in MP team game, random bot model, and fixup some bot's bugs in Quake 4.",
                        "Add `SABot`'s aas file pak of MP game maps in `Quake 4 helper dialog`."
                ),

                ChangeLog.Create("2023-03-25", 26,
                        "Using SurfaceView for rendering, remove GLSurfaceView(Testing).",
                        "Using DOOM3's Fx/Particle system implement Quake4's BSE incompletely for effects in Quake 4. The effects are bad now. Using set `bse_enabled` to 0 for disable effects.",
                        "Remove my cvar `harm_g_alwaysRun`, so set original `in_alwaysRun` to 1 for run in Quake 4.",
                        "Add simple beam model render in Prey(2006).",
                        "Optimize skybox render in Prey(2006) by `" + TextHelper.GenLinkText("https://github.com/lvonasek/PreyVR", "lvonasek/PreyVR") + "`."
                ),

                ChangeLog.Create("2023-02-20", 25,
                        "Sound with OpenSLES support(Testing).",
                        "Add backup/restore preferences support.",
                        "Add menu music playing in Prey(2006).",
                        "Add map loading music playing in Prey(2006).",
                        "Add entity visible/invisible in spirit walk mode in Prey(2006), e.g. spirit bridge.",
                        "Optimize portal render with view distance in Prey(2006)."
                ),

                ChangeLog.Create("2023-02-16", 23,
                        "Multi-threading support(Testing).",
                        "Fixup portal/skybox view in Prey(2006).",
                        "Fixup intro sound playing when start new game in Prey(2006) by `" + TextHelper.GenLinkText("https://github.com/lvonasek/PreyVR", "lvonasek/PreyVR") + "`.",
                        "Fixup player can not through first wall with spirit walk mode in `game/spindlea` beginning in Prey(2006).",
                        "Fixup render Tommy's original body when in spirit walk mode in Prey(2006).",
                        "Do not render on-screen buttons when game is loading."
                ),

                ChangeLog.Create("2023-01-10", 22,
                        "Support screen top edges with fullscreen.",
                        "Add bad skybox render in Prey(2006).",
                        "Add bad portal render in Prey(2006).",
                        "Add `deathwalk` map append support in Prey(2006)."
                ),

                ChangeLog.Create("2022-12-10", 21,
                        "Prey(2006) for DOOM3 support, game data folder named `preybase`. All levels clear, but have some bugs.",
                        "Add setup On-screen buttons position unit when config controls layout."
                ),

                ChangeLog.Create("2022-11-18", 20,
                        "Add default font for somewhere missing text in Quake 4, using cvar `harm_gui_defaultFont` to control, default is `chain`.",
                        "Implement show surface/hide surface for fixup entity render incorrect in Quake 4, e.g. AI's weapons, weapons in player view and Makron in boss level."
                ),

                ChangeLog.Create("2022-11-16", 19,
                        "Fixup middle bridge door GUI not interactive of level `game/tram1` in Quake 4.",
                        "Fixup elevator 1 with a monster GUI not interactive of level `game/process2` in Quake 4."
                ),

                ChangeLog.Create("2022-11-11", 18,
                        "Implement some debug render functions.",
                        "Add player focus GUI bracket and interactive text on HUD in Quake 4.",
                        "Automatic generating AAS file for bot of Multiplayer-Game maps is not need enable net_allowCheats when set cvar `harm_g_autoGenAASFileInMPGame` to 1 in Quake 4.",
                        "Fixed restart menu action in Quake 4.",
                        "Fixed a memory bug that can cause crash in Quake 4."
                ),

                ChangeLog.Create("2022-10-29", 17,
                        "Support Quake 4 format fonts. Other language patches will work. D3-format fonts do not need to extract no longer.",
                        "Solution of some GUIs can not interactive in Quake 4, you can try `quicksave`, and then `quickload`, the GUI can interactive. E.g. 1. A door's control GUI on bridge of level `game/tram1`, 2. A elevator's control GUI with a monster of `game/process2`."
                ),

                ChangeLog.Create("2022-10-22", 16,
                        "Add automatic load `QuickSave` when start game.",
                        "Add control Quake 4 helper dialog visible when start Quake 4 in Settings, and add `Extract Quake 4 resource` in `Other` menu.",
                        "Add setup all on-screen button opacity.",
                        "Support checking for update from GitHub.",
                        "Fixup some Quake 4 bugs: ",
                        " Fixup collision, e.g. trigger, vehicle, AI, elevator, health-station. So fixed block on last elevator in level `game/mcc_landing` and fixed incorrect collision cause killing player on elevator in `game/process1 first` and `game/process1 second` and fixed block when player jumping form vehicle in `game/convoy1`. And cvar `harm_g_useSimpleTriggerClip` is removed.",
                        " Fixup game level load fatal error and crash in `game/mcc_1` and `game/tram1b`. So all levels have not fatal error now."
                ),

                ChangeLog.Create("2022-10-15", 15,
                        "Add gyroscope control support.",
                        "Add reset onscreen button layout with fullscreen.",
                        "If running Quake 4 crash on arm32 device, trying to check `Use ETC1 compression` or `Disable lighting` for decreasing memory usage.",
                        "Fixup some Quake 4 bugs: ",
                        " Fixup start new game in main menu, now start new game is work.",
                        " Fixup loading zombie material in level `game/waste`.",
                        " Fixup AI `Singer` can not move when opening the door in level `game/building_b`.",
                        " Fixup jump down on broken floor in level `game/putra`.",
                        " Fixup player model choice and view in `Settings` menu in Multiplayer game.",
                        " Add bool cvar `harm_g_flashlightOn` for controlling gun-lighting is open/close initial, default is 1(open).",
                        " Add bool cvar `harm_g_vehicleWalkerMoveNormalize` for re-normalize `vehicle walker` movement if enable `Smooth joystick` in launcher, default is 1(re-normalize), it can fix up move left-right."
                ),

                ChangeLog.Create("2022-10-29", 13,
                        "Fixup Strogg health station GUI interactive in `Quake 4`.",
                        "Fixup skip cinematic in `Quake 4`.",
                        "If `harm_g_alwaysRun` is 1, hold `Walk` key to walk in `Quake 4`.",
                        "Fixup level map script fatal error or bug in `Quake 4`(All maps have not fatal errors no longer, but have some bugs yet.).",
                        " `game/mcc_landing`: Player collision error on last elevator. You can jump before elevator ending or using `noclip`.",
                        " `game/mcc_1`: Loading crash after last level ending. Using `map game/mcc_1` to reload.",
                        " `game/convoy1`: State error is not care no longer and ignore. But sometimes has player collision error when jumping form vehicle, using `noclip`.",
                        " `game/putra`: Script fatal error has fixed. But can not down on broken floor, using `noclip`.",
                        " `game/waste`: Script fatal error has fixed.",
                        " `game/process1 first`: Last elevator has ins collision cause killing player. Using `god`. If tower's elevator GUI not work, using `teleport tgr_endlevel` to next level directly.",
                        " `game/process1 second`: Second elevator has incorrect collision cause killing player(same as `game/process1 first` level). Using `god`.",
                        " `game/tram_1b`: Loading crash after last level ending. Using `map game/tram_1b` to reload.",
                        " `game/core1`: Fixup first elevator platform not go up.",
                        " `game/core2`: Fixup entity rotation."
                ),

                ChangeLog.Create("2022-07-19", 12,
                        "`Quake 4` in DOOM3 engine support. Also see `" + TextHelper.GenLinkText("https://github.com/jmarshall23/Quake4Doom", null) + "`. Now can play most levels, but some levels has error.",
                        "Quake 4 game data folder named `q4base`, also see `" + TextHelper.GenLinkText("https://store.steampowered.com/app/2210/Quake_4/", null) + "`.",
                        "Fix `Rivensin` and `Hardcorps` mod load game from save game.",
                        "Add console command history record.",
                        "On-screen buttons layer's resolution always same to device screen.",
                        "Add volume key map config(Enable `Map volume keys` to show it)."
                ),

                ChangeLog.Create("2022-06-30", 11,
                        "Add `Hardcorps` mod library support, game path name is `hardcorps`, if play the mod, first suggest to close `Smooth joystick` in `Controls` tab panel, more view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/hardcorps", null) + "`.",
                        "In `Rivensin` mod, add bool Cvar `harm_pm_doubleJump` to enable double-jump(From `hardcorps` mod source code, default disabled).",
                        "In `Rivensin` mod, add bool Cvar `harm_pm_autoForceThirdPerson` for auto set `pm_thirdPerson` to 1 after level load end when play original DOOM3 maps(Default disabled).",
                        "In `Rivensin` mod, add float Cvar `harm_pm_preferCrouchViewHeight` for view poking out some tunnel's ceil when crouch(Default 0 means disabled, and also can set `pm_crouchviewheight` to a smaller value).",
                        "Add on-screen button config page, and reset some on-screen button keymap to DOOM3 default key.",
                        "Add menu `Cvar list` in `Other` menu for list all new special `Cvar`."
                ),

                ChangeLog.Create("2022-06-23", 10,
                        "Add `Rivensin` mod library support, game path name is `rivensin`, more view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/ruiner", null) + "`.",
                        "The `Rivensin` game library support load DOOM3 base game map. But first must add include original DOOM3 all map script into `doom_main.script` of `Rivensin` mod file.",
                        "Add weapon panel keys configure.",
                        "Fix file access permission grant on Android 10."
                ),

                ChangeLog.Create("2022-06-15", 9,
                        "Fix file access permission grant on Android 11+."
                ),

                ChangeLog.Create("2022-05-19", 8,
                        "Compile armv8-a 64 bits library.",
                        "Set FPU neon is default on armv7-a, and do not compile old armv5e library and armv7-a vfp.",
                        "Fix input event when modal MessageBox is visible in game.",
                        "Add cURL support for downloading in multiplayer game.",
                        "Add weapon on-screen button panel."
                ),

                ChangeLog.Create("2022-05-05", 7,
                        "Fix shadow clipped.",
                        "Fix sky box.",
                        "Fix fog and blend light.",
                        "Fix glass reflection.",
                        "Add `texgen` shader for like `D3XP` hell level sky.",
                        "Fix translucent object. i.e. window glass, translucent Demon in `Classic DOOM` mod.",
                        "Fix dynamic texture interaction. i.e. rotating fans.",
                        "Fix `Berserk`, `Grabber`, `Helltime` vision effect(First set cvar `harm_g_skipBerserkVision`, `harm_g_skipWarpVision` and `harm_g_skipHelltimeVision` to 0).",
                        "Fix screen capture image when quick save game or mission tips.",
                        "Fix machine gun's ammo panel.",
                        "Add light model setting with `Phong` and `Blinn-Phong` when render interaction shader pass(string cvar `harm_r_lightingModel`).",
                        "Add specular exponent setting in light model(float cvar `harm_r_specularExponent`).",
                        "Default using program internal OpenGL shader.",
                        "Reset extras virtual button size, and add Console(~) key.",
                        "Add cvar `harm_r_shadowCarmackInverse` to change general Z-Fail stencil shadow or `Carmack-Inverse` Z-Fail stencil shadow.",
                        "Add `Back` key function setting, add 3-Click to exit."
                ),

                ChangeLog.Create("2020-08-25", 5,
                        "Fix video playing.",
                        "Choose game library when load other game mod, more view in `Help` menu."
                ),

                ChangeLog.Create("2020-08-21", 3,
                        "Fix game audio sound playing(Testing).",
                        "Add launcher orientation setting on `CONTROLS` tab."
                ),

                ChangeLog.Create("2020-08-17", 2,
                        "Uncheck 4 checkboxs, default value is 0(disabled).",
                        "Hide software keyboard when open launcher activity.",
                        "Check `WRITE_EXTERNAL_STORAGE` permission when start game or edit config file.",
                        "Add game data directory chooser.",
                        "Add `Save settings` menu if you only change settings but don't want to start game.",
                        "UI editor can hide navigation bar if checked `Hide navigation bar`(the setting must be saved before do it).",
                        "Add `Help` menu."
                ),

                ChangeLog.Create("2020-07-20", 1,
                        "Compile `DOOM3:RoE` game library named `libd3xp`, game path name is `d3xp`, more view in `" + TextHelper.GenLinkText("https://store.steampowered.com/app/9070/DOOM_3_Resurrection_of_Evil/", null) + "`.",
                        "Compile `Classic DOOM3` game library named `libcdoom`, game path name is `cdoom`, more view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/classic-doom-3", null) + "`.",
                        "Compile `DOOM3-BFG:The lost mission` game library named `libd3le`, game path name is `d3le`, need `d3xp` resources(+set fs_game_base d3xp), more view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/the-lost-mission", null) + "`(now fix stack overflow when load model `models/mapobjects/hell/hellintro.lwo` of level `game/le_hell` map on Android).",
                        "Clear vertex buffer for out of graphics memory(integer cvar `harm_r_clearVertexBuffer`).",
                        "Skip visual vision for `Berserk Powerup` on `DOOM3`(bool cvar `harm_g_skipBerserkVision`).",
                        "Skip visual vision for `Grabber` on `D3 RoE`(bool cvar `harm_g_skipWarpVision`).",
                        "Skip visual vision for `Helltime Powerup` on `D3 RoE`(bool cvar `harm_g_skipHelltimeVision`).",
                        "Add support to run on background.",
                        "Add support to hide navigation bar.",
                        "Add RGBA4444 16-bits color.",
                        "Add config file editor."
                ),
        };
        return CHANGES;
    }
}
