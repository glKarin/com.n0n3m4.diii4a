package com.karin.idTech4Amm.misc;

import android.os.Build;
import android.text.Html;
import android.content.Context;

import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.Constants;

import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Collections;
import java.util.Collection;

/**
 * Global text define
 */
public final class TextHelper
{
	public static final boolean USING_HTML = true;

	public static String GetDialogMessageEndl()
	{
		return USING_HTML ? "<br>" : "\n";
	}

	public static String FormatDialogMessageSpace(String space)
	{
		return USING_HTML ? space.replaceAll(" ", "&nbsp;") : space;
	}

	public static String FormatDialogMessageHeaderSpace(String space)
	{
		if(!USING_HTML)
			return space;
		int i = 0;
		for(; i < space.length(); i++)
		{
			if(space.charAt(i) != ' ')
				break;
		}
		if(i == 0)
			return space;
		return FormatDialogMessageSpace(space.substring(0, i)) + space.substring(i);
	}

    public static CharSequence GetDialogMessage(String text)
    {
        return USING_HTML ? Html.fromHtml(text) : text;      
    }
    
    public static class ChangeLog
    {
        public String date;
        public int release;
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
            return GenString(GetDialogMessageEndl());
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
                        sb.append(FormatDialogMessageSpace("  * ")).append(str);
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
	}
    
    public static String GenLinkText(String link, String name)
    {
        StringBuilder sb = new StringBuilder();
        if(USING_HTML)
        {
            String nameText = name != null && !name.isEmpty() ? name : link;
            sb.append("<a href='").append(link).append("'>").append(nameText).append("</a>");
        }
        else
        {
            if(name != null && !name.isEmpty())
                sb.append(name).append('(').append(link).append(')');
            else
                sb.append(link);
        }
        return sb.toString();
    }
 
	public static CharSequence GetUpdateText(Context context)
	{
        StringBuilder sb = new StringBuilder();
        final String[] INFOS = {
            Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")",
            Constants.CONST_NAME,
            "Update: " + ContextUtility.GetAppVersion(context) + (ContextUtility.BuildIsDebug(context) ? "(debug)" : ""),
            "Release: " + Constants.CONST_RELEASE + " (R" + Constants.CONST_UPDATE_RELEASE + ")",
            "Dev: " + GenLinkText("mailto:" + Constants.CONST_EMAIL, Constants.CONST_DEV),
            "Changes: ",
        };
        final String endl = GetDialogMessageEndl();
        for(String str : INFOS)
        {
            if(null != str)
                sb.append(str);
            sb.append(endl);
        }
        for(String str : Constants.CONST_CHANGES)
        {
            if(null != str)
                sb.append(FormatDialogMessageSpace("  * ")).append(str);
            sb.append(endl);
        }
		return GetDialogMessage(sb.toString());
	}

	public static CharSequence GetHelpText()
	{
        StringBuilder sb = new StringBuilder();
        final String[] HELPS = {
                "Launch game: ",
                " 1. Putting your PC game data files(external folder) to launcher setting `Game working directory`(default `/sdcard/diii4a`).",
                " 2. Click left icon or right game name text of launcher status bar for choosing game, and select game mod in tab `GENERAL`'s `Game`.",
                " 3. Finally START GAME.",
                null,
                "Folder name of games/mods:",
                " DOOM3: base",
                " DOOM3-Resurrection of Evil: d3xp",
                " DOOM3-The lost mission: d3le",
                " Classic DOOM3: cdoom",
                " Rivensin: rivensin",
                " Hardcorps: hardcorps",
                " Quake4: q4base",
                " Prey(2006): preybase",
                null,
            "For playing Prey(2006)(Thanks for `" + GenLinkText("https://github.com/jmarshall23", "jmarshall") + "`'s `" + GenLinkText("https://github.com/jmarshall23/PreyDoom", "PreyDoom") + "`): ",
            " 1. Putting PC Prey game data file to `preybase` folder and START directly.",
            " *. Some problems solution: e.g. using cvar `harm_g_translateAlienFont` to translate Alien text on GUI.",
            " *. Exists bugs: e.g. some incorrect collision(using `noclip`), some menu draw(Tab window), some GUIs not work(Music CD in RoadHouse).",
            " *. Because of tab window UI not support, Settings UI is not work, must edit `preyconfig.cfg` for binding extras key.",
            "  bind \"Your key of spirit walk\" \"_impulse54\"",
            "  bind \"Your key of second mode attack of weapons\" \"_attackAlt\"",
            "  bind \"Your key of toggle lighter\" \"_impulse16\"",
            "  bind \"Your key of drop\" \"_impulse25\"",
            " *. If not sound when start new game and load `roadhouse` map, try to press `ESC` key back to main menu and then press `ESC` key to back game, then sound can be played.",
            null,
			"For playing Quake 4(Thanks for `" + GenLinkText("https://github.com/jmarshall23", "jmarshall") + "`'s `" + GenLinkText("https://github.com/jmarshall23/Quake4Doom", "Quake4Doom") + "`): ",
			" 1. Putting PC Quake 4 game data file to `q4base` folder and START directly.",
            " 2. Extract Quake 4 patch resource to `q4base` game data folder if need(in menu `Other` -> `Extract resource`).",
            "  (a). Quake 3 bot files(If you want to add bots in Multiplayer-Game, using command `addbot <bot_file>` or `fillbots` after enter map in console, or set `harm_si_autoFillBots` to 1 for automatic fill bots).",
            "  (b). `SABot` MP game map aas files(for bots in MP game).",
            " 3. Then start game directly or choose map level, all levels is working.",
            " *. If running crash on arm32 or low-memory device, trying to check `Use ETC1 compression` or `Disable lighting` for decreasing memory usage.",
            " *. Exists bugs: e.g. some GUIs interface, script error on some levels",
            " *. Effect system: Quake4 using new advanced `BSE` particle system, it not open-source(`jmarshall` has realized and added by decompiling `ETQW`'s BSE binary file, also see `" + GenLinkText("https://github.com/jmarshall23/Quake4BSE", "jmarshall23/Quake4BSE") + "`, but it not work yet.). Now implementing a OpenBSE with DOOM3 original FX/Particle system, some effects can played, but has incorrect render.",
			" *. Multiplayer-Game: It is working well with bots(`jmarshall` added Q3-bot engine, but need bots decl file and Multiplayer-Game map AAS file, now set cvar `harm_g_autoGenAASFileInMPGame` to 1 for generating a bad AAS file when loading map in Multiplayer-Game and not valid AAS file in current map, you can also put your MP map's AAS file to `maps/mp` folder).",
            null,
            "Multi-threading and some GLSL shader using `" + GenLinkText("https://github.com/emileb/d3es-multithread", "emileb/d3es-multithread") + "`.",
            null,
            // "On Android 11+, because of `Scoped-Storage`, must grant `Allow management of all files` permission.",
            "On Android 10+, if game files loading slowly, suggest to set game data directory is under `/sdcard/Android/data/" + Constants.CONST_PACKAGE + "/`.",
            null,
            "All special `CVAR`s are start with `harm_`.",
            "More Cvar's detail view in menu `Other` -> `Cvar list`.",
            null,
            "If game running crash(white screen): ",
            " 1. Make sure to allow `WRITE_EXTERNAL_STORAGE` permission.",
            " 2. Make sure `Game working directory` is right.",
            " 3. Uncheck 4th checkbox named `Use ETC1(or RGBA4444) cache` or clear ETC1 texture cache file manual on resource folder(exam: /sdcard/diii4a/(base/d3xp/d3le/cdoom/rivensin/or...)/dds).",
            null,
            "If game is crash with flash-screen when playing a period of time: ",
            " 1. Out of graphics memory: `Clear vertex buffer` suggest to select 3rd or 2nd for clear vertex buffer every frame! If you select 1st, it will be same as original apk(ver 1.1.0 at 2013). It should work well on `Adreno` GPU of `Snapdragon`. More view in game, on DOOM3 console, cvar named `harm_r_clearVertexBuffer`.",
            null,
            "If want to load other mod: ",
            " 1. Input folder name of game mod to editor that under `User special` checkbox.",
            " 2. Check `User special` checkbox. `Commandline` will show `+set fs_game (the game mod)`.",
            " 3. If may want to choose game library, click `GameLib` and choose a game library. `Commandline` will show `+set harm_fs_gameLibPath (selected library path)`.",
			null,
			"The `Rivensin` game library support load DOOM3 base game map. But first must add include original DOOM3 all map script into `doom_main.script` of `Rivensin` mod file.",
			" 1. Edit `doom_main.script` in pak archive file(in `script/` folder) or external folder of file system.",
			" 2. Add include all map's script to `doom_main.script`.",
			"  2-1. Find text line `// map specific character scripts`",
			"  2-2. Put these code below the commented line(These code can found in `script/doom_main.script` of DOOM3 base game pak archive): ",
			"    #include \"script/map_admin1.script\"",
			"    #include \"script/map_alphalabs1.script\"",
			"    #include \"script/map_alphalabs2.script\"",
			"    #include \"script/map_alphalabs3.script\"",
			"    #include \"script/map_alphalabs4.script\"",
			"    #include \"script/map_caves.script\"",
			"    #include \"script/map_caves2.script\"",
			"    #include \"script/map_comm1.script\"",
			"    #include \"script/map_commoutside_lift.script\"",
			"    #include \"script/map_commoutside.script\"",
			"    #include \"script/map_cpu.script\"",
			"    #include \"script/map_cpuboss.script\"",
			"    #include \"script/map_delta1.script\"",
			"    #include \"script/map_delta2a.script\"",
			"    #include \"script/map_delta2b.script\"",
			"    #include \"script/map_delta3.script\"",
			"    #include \"script/map_delta5.script\"",
			"    #include \"script/map_enpro.script\"",
			"    #include \"script/map_hell1.script\"",
			"    #include \"script/map_hellhole.script\"",
			"    #include \"script/map_recycling1.script\"",
			"    #include \"script/map_recycling2.script\"",
			"    #include \"script/map_site3.script\"",
			"    #include \"script/map_marscity1.script\"",
			"    #include \"script/map_marscity2.script\"",
			"    #include \"script/map_mc_underground.script\"",
			"    #include \"script/map_monorail.script\"",
			"  3-3. Choose `Rivensin` mod and start game in game launcher.",
			"  3-4. Open console, and then using `map game/xxx` to load DOOM3 base game map.",
        };
        final String endl = GetDialogMessageEndl();
        for(String str : HELPS)
        {
            if(null != str)
			{
				if(str.startsWith(" "))
					sb.append(FormatDialogMessageHeaderSpace(str));
				else
					sb.append("* ").append(str);
			}
            sb.append(endl);
        }
		return GetDialogMessage(sb.toString());
	}

	public static CharSequence GetAboutText(Context context)
	{
        StringBuilder sb = new StringBuilder();
        final String[] ABOUTS = {
            Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")",
            Constants.CONST_NAME,
            "Changes by " + GenLinkText(Constants.CONST_DEVELOPER, Constants.CONST_DEV)
            + "&lt;" + GenLinkText("mailto:" + Constants.CONST_EMAIL, Constants.CONST_EMAIL) + "&gt;",
            "Update: " + ContextUtility.GetAppVersion(context) + ("(API " + Build.VERSION.SDK_INT + ")") + (ContextUtility.BuildIsDebug(context) ? "(debug)" : ""),
            "Release: " + Constants.CONST_RELEASE + " (R" + Constants.CONST_UPDATE_RELEASE + ")",
            null,
            "Rename from `DIII4A++`, base on original `n0n3m4`'s `DIII4A`.",
            "idTech4 engine's games support on Android.",
            "e.g. `DOOM 3`, `DOOM 3 RoE`, `Quake 4`, Prey(2006), and some mods.",
            null,
            "Source in `assets/source` folder in APK file.",
            " `DIII4A.source.tgz`: launcher frontend source and game source, game source and OpenGLES2.0 shader source in `/Q3E/src/main/jni/doom3` of archive package.",
            null,
            "Homepage: ",
            "Github: " + GenLinkText(Constants.CONST_MAIN_PAGE, null),
            "F-Droid: " + GenLinkText(Constants.CONST_FDROID, null),
            "Tieba: " + GenLinkText(Constants.CONST_TIEBA, null),
            "XDA: " + GenLinkText(Constants.CONST_DEVELOPER_XDA, "karin_zhao"),
            null,
            "Special thanks: ",
            GenLinkText("https://4pda.ru/forum/index.php?showuser=7653620", "Sir Cat") + "@" + GenLinkText("https://4pda.ru/forum/index.php?showtopic=929753", "4PDA forum"),
            GenLinkText("https://4pda.ru/forum/index.php?showuser=5043340", "ALord7") + "@" + GenLinkText("https://4pda.to/forum/index.php?showtopic=330329", "4PDA forum"),
        };
        final String endl = GetDialogMessageEndl();
        for(String str : ABOUTS)
        {
            if(null != str)
                sb.append(/*"  * " + */str);
            sb.append(endl);
        }
		return GetDialogMessage(sb.toString());
	}

    public static CharSequence GetChangesText()
    {
        final ChangeLog[] CHANGES = {
            ChangeLog.Create(Constants.CONST_RELEASE, Constants.CONST_UPDATE_RELEASE, Constants.CONST_CHANGES),

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
                        "Optimize skybox render in Prey(2006) by " + TextHelper.GenLinkText("https://github.com/lvonasek/PreyVR", "lvonasek/PreyVR") + "`."
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
                             "Add light model setting with `Phong` and `Blinn-Phong` when render interaction shader pass(string cvar `harm_r_lightModel`).",
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
                         "Compile `DOOM3:RoE` game library named `libd3xp`, game path name is `d3xp`, more view in `" + GenLinkText("https://store.steampowered.com/app/9070/DOOM_3_Resurrection_of_Evil/", null) + "`.",
                         "Compile `Classic DOOM3` game library named `libcdoom`, game path name is `cdoom`, more view in `" + GenLinkText("https://www.moddb.com/mods/classic-doom-3", null) + "`.",
                         "Compile `DOOM3-BFG:The lost mission` game library named `libd3le`, game path name is `d3le`, need `d3xp` resources(+set fs_game_base d3xp), more view in `" + GenLinkText("https://www.moddb.com/mods/the-lost-mission", null) + "`(now fix stack overflow when load model `models/mapobjects/hell/hellintro.lwo` of level `game/le_hell` map on Android).",
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
    
        StringBuilder sb = new StringBuilder();
        final String endl = GetDialogMessageEndl();
        for(ChangeLog changeLog : CHANGES)
        {
            if(null != changeLog)
                sb.append(changeLog.GenString(endl));
            sb.append(endl);
        }
        return GetDialogMessage(sb.toString());
    }
    
    private static class Cvar
    {
        public String name;
        public String type;
        public String defaultValue;
        public String description;
        public List<Value> values;
        public static class Value {
            public String value;
            public String desc;
            public Value(String value, String desc)
            {
                this.value = value;
                this.desc = desc;
            }

            public String GenString(String endl)
            {
                StringBuilder sb = new StringBuilder();
                sb.append(FormatDialogMessageSpace("    ")).append(value).append(" - ").append(desc);
                sb.append(endl);
                return sb.toString();
            }

            @Override
            public String toString()
            {
                return GenString(GetDialogMessageEndl());
            }
        }

        public Cvar Name(String name)
        {
            this.name = name;
            return this;
        }

        public Cvar DefaultValue(String def)
        {
            this.defaultValue = def;
            return this;
        }

        public Cvar Type(String type)
        {
            this.type = type;
            return this;
        }

        public Cvar Description(String desc)
        {
            this.description = desc;
            return this;
        }

        public Cvar Value(String value, String desc)
        {
            if(values == null)
                values = new ArrayList<>();
            values.add(new Value(value, desc));
            return this;
        }

        public Cvar Values(String...args)
        {
            for(int i = 0; i < args.length - 1; i += 2)
                Value(args[i], args[i + 1]);
            return this;
        }

        public String GenString(String endl)
        {
            StringBuilder sb = new StringBuilder();
            sb.append(FormatDialogMessageSpace("  * ")).append(name).append(" (").append(type).append(") default: ").append(defaultValue);
            sb.append(FormatDialogMessageSpace("    ")).append(description);
            sb.append(endl);
            if(null != values && !values.isEmpty())
            {
                for(Value str : values)
                {
                    sb.append(FormatDialogMessageSpace("    ")).append(str.GenString(endl));
                }
            }
            return sb.toString();
        }

        @Override
        public String toString()
        {
            return GenString(GetDialogMessageEndl());
        }

        public static Cvar Create(String name, String type, String def, String desc, String...args)
        {
            Cvar res = new Cvar();
            res.Name(name)
                .Type(type)
                .DefaultValue(def)
                .Description(desc)
                ;
            if(args.length >= 2)
                res.Values(args);
            return res;
        }
    }
    
    public static CharSequence GetCvarText()
    {
        final Cvar[] RENDSRER_CVARS = {
            Cvar.Create("harm_r_clearVertexBuffer", "integer", "2", "Clear vertex buffer on every frame.",
                        "0", "Not clear(original).",
						"1", "Force clear.",
						"2", "Force clear including shutdown."
                        ),
            Cvar.Create("harm_r_shadowCarmackInverse", "bool", "0", "Stencil shadow using Carmack-Inverse."),
            Cvar.Create("harm_r_lightModel", "string", "phong", "Light model when draw interactions.",
                        "phong", "Phong.",
						"blinn_phong", "Blinn-Phong."
                        ),
            Cvar.Create("harm_r_specularExponent", "float", "4.0", "Specular exponent in interaction light model."),
            Cvar.Create("harm_r_shaderProgramDir", "string", "glslprogs", "Special external GLSL shader program directory path"),
            Cvar.Create("harm_r_maxAllocStackMemory", "integer", "524288", "Control allocate temporary memory when load model data on Android, if less than this `byte` value, call `alloca` in stack memory, else call `malloc`/`calloc` in heap memory.",
                        "0", "Always heap",
						"Negative", "Always stack(original).",
						"Positive", "Max stack memory limit."
                        ),
        };
        final Cvar[] COMMON_CVARS = {
            Cvar.Create("harm_fs_gameLibPath", "string", "", "Special game dynamic library."),
            Cvar.Create("harm_fs_gameLibDir", "string", "", "Special game dynamic library directory path(default is empty, means using apk install libs directory path."),
            Cvar.Create("harm_com_consoleHistory", "integer", "1", "Save/load console history.",
                "0", "disable",
                "1", "loading in engine initialization, and saving in engine shutdown",
                "2", "loading in engine initialization, and saving in every e executing"
            ),
        };
        final Cvar[] GAME_CVARS = {
            Cvar.Create("harm_g_skipBerserkVision", "bool", "0", "Skip render berserk vision for power up."),
                Cvar.Create("harm_pm_fullBodyAwareness", "bool", "0", "Enables full-body awareness."),
                Cvar.Create("harm_pm_fullBodyAwarenessOffset", "vector3", "0 0 0", "Full-body awareness offset(<forward-offset> <side-offset> <up-offset>)."),
        };
        final Cvar[] D3XP_CVARS = {
            Cvar.Create("harm_g_skipWarpVision", "bool", "0", "Skip render warp vision for grabber dragging."),
            Cvar.Create("harm_g_skipHelltimeVision", "bool", "0", "Skip render helltime vision for powerup."),
        };
        final Cvar[] D3LE_CVARS = {
            Cvar.Create("harm_g_skipWarpVision", "bool", "0", "Skip render warp vision for grabber dragging."),
            Cvar.Create("harm_g_skipHelltimeVision", "bool", "0", "Skip render helltime vision for powerup."),
        };
        final Cvar[] CDOOM_CVARS = {
            Cvar.Create("harm_g_skipBerserkVision", "bool", "0", "Skip render berserk vision for power up."),
        };
        final Cvar[] RIVENSIN_CVARS = {
            Cvar.Create("harm_g_skipBerserkVision", "bool", "0", "Skip render berserk vision for power up."),
            Cvar.Create("harm_pm_doubleJump", "bool", "0", "Enable double-jump."),
            Cvar.Create("harm_pm_autoForceThirdPerson", "bool", "0", "Force set third person view after game level load end."),
            Cvar.Create("harm_pm_preferCrouchViewHeight", "float", "32", "Set prefer crouch view height in Third-Person(suggest 32 - 39, less or equals 0 to disable)."),
        };
        final Cvar[] HARDCORPS_CVARS = {
            Cvar.Create("harm_g_skipBerserkVision", "bool", "0", "Skip render berserk vision for power up."),
        };
        final Cvar[] QUAKE4_CVARS = {
            Cvar.Create("harm_g_autoGenAASFileInMPGame", "bool", "1", "For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic."),
            Cvar.Create("harm_g_vehicleWalkerMoveNormalize", "bool", "1", "Re-normalize vehicle walker movement."),
            Cvar.Create("harm_gui_defaultFont", "string", "chain", "Default font name.",
                    "chain", "fonts/chain",
                    "lowpixel", "fonts/lowpixel",
                    "marine", "fonts/marine",
                    "profont", "fonts/profont",
                    "r_strogg", "fonts/r_strogg",
                    "strogg", "fonts/strogg"
            ),
            Cvar.Create("harm_si_autoFillBots", "bool", "0", "Automatic fill bots after map loaded in multiplayer game(0: disable; other number: bot num)."),
            Cvar.Create("addbot", "string", "", "adds a multiplayer bot(support `tab` complete, exam. addbot bot_name1 bot_name2 ...)."),
            Cvar.Create("fillbots", "integer", "", "fill bots(empty argument to fill max bots num, exam. fillbots 8)."),
            Cvar.Create("harm_g_mutePlayerFootStep", "bool", "1", "Mute player's footstep sound."),
        };
        final Cvar[] PREY_CVARS = {
                Cvar.Create("harm_g_translateAlienFont", "string", "fonts", "Setup font name for automatic translate `alien` font text of GUI(empty to disable).",
                        "fonts", "fonts",
                        "fonts/menu", "fonts/menu",
                        "\"\"", "Disable"
                ),
        };
        
        Map<String, Cvar[]> cvarMap = new LinkedHashMap<>();
        cvarMap.put("Renderer", RENDSRER_CVARS);
        cvarMap.put("Common", COMMON_CVARS);
        cvarMap.put("Base game", GAME_CVARS);
        cvarMap.put("DOOM3 RoE", D3XP_CVARS);
        cvarMap.put("Classic DOOM", CDOOM_CVARS);
        cvarMap.put("The lost mission", D3LE_CVARS);
        cvarMap.put("Rivensin mod", RIVENSIN_CVARS);
        cvarMap.put("Hardcorps mod", HARDCORPS_CVARS);
        cvarMap.put("Quake 4", QUAKE4_CVARS);
        cvarMap.put("Prey(2006)", PREY_CVARS);

        StringBuilder sb = new StringBuilder();
        final String endl = GetDialogMessageEndl();
        for(Map.Entry<String, Cvar[]> item : cvarMap.entrySet())
        {
            sb.append("------- ").append(item.getKey()).append(" -------");
            sb.append(endl);
            for(Cvar cvar : item.getValue())
                sb.append(cvar.GenString(endl));
            sb.append(endl);
        }
        return GetDialogMessage(sb.toString());
    }
    
    public static String Join(Object[] args, String sep)
    {
        StringBuilder sb = new StringBuilder();
        for(int i = 0; i < args.length; i++)
        {
            Object o = args[i];
            if(null == o)
                continue;
            sb.append(o);
            if(i < args.length - 1)
                sb.append(null != sep ? sep : "");
        }
        return sb.toString();
    }

    public static String Join(Collection<?> args, String sep)
    {
        return Join(args.toArray(), sep);
    }

    public static String Join(String sep, String...args)
    {
        return Join(args, sep);
    }
    
	private TextHelper() {}
}

