package com.karin.idTech4Amm.misc;

import android.text.Html;
import android.content.Context;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.Constants;
import com.karin.idTech4Amm.lib.KCVar;
import com.karin.idTech4Amm.lib.KCVarSystem;
import com.n0n3m4.q3e.Q3ELang;

import java.util.Map;

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
        final String Update_ = Q3ELang.tr(context, R.string.update_);
        final String Release_ = Q3ELang.tr(context, R.string.release_);
        final String Build_ = Q3ELang.tr(context, R.string.build_);
        final String Dev_ = Q3ELang.tr(context, R.string.dev_);
        final String Changes_ = Q3ELang.tr(context, R.string.changes_);
        final String[] INFOS = {
            Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")",
            // Constants.CONST_NAME,
            Update_ + " " + ContextUtility.GetAppVersion(context) + (ContextUtility.BuildIsDebug(context) ? "(debug)" : ""),
            Release_ + " " + Constants.CONST_RELEASE + " (R" + Constants.CONST_UPDATE_RELEASE + " - " + Constants.CONST_CODE_ALIAS + ")",
            Build_ + " " + Constants.GetBuildTime("yyyy-MM-dd HH:mm:ss.SSS") + ("(API " + Constants.GetBuildSDKVersion() + ")"),
            Dev_ + " " + GenLinkText("mailto:" + Constants.CONST_EMAIL, Constants.CONST_DEV),
            Changes_ + " ",
        };
        final String endl = GetDialogMessageEndl();
        for(String str : INFOS)
        {
            if(null != str)
                sb.append(str);
            sb.append(endl);
        }
        for(String str : Constants.CONST_CHANGES())
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
                "Folder name of games/mods can view by `Game path tips` on launcher",
                null,
            "For playing Prey(2006)(Based on `" + GenLinkText("https://github.com/jmarshall23", "jmarshall") + "`'s `" + GenLinkText("https://github.com/jmarshall23/PreyDoom", "PreyDoom") + "`): ",
            " 1. Putting PC Prey game data file to `preybase` folder and START directly.",
            " *. Some problems solution: e.g. using cvar `harm_ui_translateAlienFont` to translate Alien text on GUI.",
            " *. Exists bugs: e.g. some incorrect collision(using `noclip`), some GUIs not work(Music CD in RoadHouse).",
            " *. If settings UI is not work, can edit `preyconfig.cfg` for binding extras key.",
            "  bind \"Your key of spirit walk\" \"_impulse54\"",
            "  bind \"Your key of second mode attack of weapons\" \"_attackAlt\"",
            "  bind \"Your key of toggle lighter\" \"_impulse16\"",
            "  bind \"Your key of drop\" \"_impulse25\"",
            null,
			"For playing Quake 4(Based on `" + GenLinkText("https://github.com/jmarshall23", "jmarshall") + "`'s `" + GenLinkText("https://github.com/jmarshall23/Quake4Doom", "Quake4Doom") + "`): ",
			" 1. Putting PC Quake 4 game data file to `q4base` folder and START directly.",
            " *. If running crash on arm32 or low-memory device, trying to check `Use ETC1 compression` or `Disable lighting` for decreasing memory usage.",
            " *. Effect system: Quake4 using new advanced `BSE` particle system, it not open-source(`jmarshall` has realized and added by decompiling `ETQW`'s BSE binary file, also see `" + GenLinkText("https://github.com/jmarshall23/Quake4BSE", "jmarshall23/Quake4BSE") + "`, but it not work yet.). Now implementing a OpenBSE with DOOM3 original FX/Particle system, some effects can played, but has incorrect render.",
            " 2. Bot mod in Multi-Player game: ",
            " *. Extract `q4base/sabot_a9.pk4` file in apk to Quake4 game data folder, it includes some defs, scripts and MP game map AAS file.",
            " *. Set cvar `harm_g_autoGenAASFileInMPGame` to 1 for generating a bad AAS file when loading map in Multiplayer-Game and not valid AAS file in current map, you can also put your MP map's AAS file to `maps/mp` folder(botaas32).",
            " *. Set `harm_si_autoFillBots` to 1 for automatic fill bots when start MP game.",
            " *. Execute `addbots` for add multiplayer bot.",
            " *. Execute `fillbots` for auto fill multiplayer bots.",
            null,
            "Multi-threading and some GLSL shader using `" + GenLinkText("https://github.com/emileb/d3es-multithread", "emileb/d3es-multithread") + "`.",
            null,
            "On Android 10+, if game files loading slowly, suggest to set game data directory is under `/sdcard/Android/data/" + Constants.CONST_PACKAGE + "/`.",
            null,
            "All special `CVAR`s are start with `harm_`.",
            "More Cvar's detail view in menu `Other` -> `Cvar list`.",
            null,
            "If game is crash with flash-screen when playing a period of time: ",
            " 1. Out of graphics memory: `Clear vertex buffer` suggest to select 3rd or 2nd for clear vertex buffer every frame! If you select 1st, it will be same as original apk(ver 1.1.0 at 2013). It should work well on `Adreno` GPU of `Snapdragon`. More view in game, on DOOM3 console, cvar named `harm_r_clearVertexBuffer`.",
            null,
            "If want to load other mod: ",
            " 1. Input folder name of game mod to editor that under `User special` checkbox.",
            " 2. Check `User special` checkbox. `Commandline` will show `+set fs_game (the game mod)`.",
            " 3. If may want to choose game library, click `GameLib` and choose a game library. `Commandline` will show `+set harm_fs_gameLibPath (selected library path)`.",
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
        final String Update_ = Q3ELang.tr(context, R.string.update_);
        final String Release_ = Q3ELang.tr(context, R.string.release_);
        final String Build_ = Q3ELang.tr(context, R.string.build_);
        final String Dev_ = Q3ELang.tr(context, R.string.dev_);
        StringBuilder sb = new StringBuilder();
        final String[] ABOUTS = {
            Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")",
            Constants.CONST_NAME,
            Dev_ + " " + GenLinkText(Constants.CONST_DEVELOPER, Constants.CONST_DEV)
            + "&lt;" + GenLinkText("mailto:" + Constants.CONST_EMAIL, Constants.CONST_EMAIL) + "&gt;",
            Update_ + " " + ContextUtility.GetAppVersion(context) + (ContextUtility.BuildIsDebug(context) ? "(debug)" : ""),
            Release_ + " " + Constants.CONST_RELEASE + " (R" + Constants.CONST_UPDATE_RELEASE + " - " + Constants.CONST_CODE_ALIAS + ")",
            Build_ + " " + Constants.GetBuildTime("yyyy-MM-dd HH:mm:ss.SSS") + ("(API " + Constants.GetBuildSDKVersion() + ")"),
            null,
            "Rename from `DIII4A++`, base on original `n0n3m4`'s `DIII4A`.",
            "Open source engine's games support on Android.",
            "1. idTech4 engine:",
            " e.g. `DOOM 3`, `DOOM 3 RoE`, `Quake 4`, `Prey(2006)`, `The Dark Mod`, `DOOM 3 BFG`, and some mods(e.g. `The Lost Mission`).",
            "2. Other idTech engine:",
            " e.g. `Return to Castle Wolfenstein`, `Quake III`, `Quake II`, `Quake`, `GZDOOM`, `Wolfenstein: Enemy Territory`, `RealRTCW`, `FTEQW`, `OpenJK`.",
            "3. Other open source engine:",
            " e.g. `Serious Sam Classic`, `Xash3D`.",
            null,
            "Source url in `assets/source` folder in APK file.",
            " `DIII4A.source.tgz.url`: launcher frontend source and game source, game source and OpenGLES2.0/3.0 shader source in `/Q3E/src/main/jni/doom3` of archive package.",
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
            GenLinkText("https://github.com/lvonasek", "Luboš Vonásek") + "@" + GenLinkText("https://github.com/lvonasek/PreyVR", "PreyVR"),
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
        final ChangeLog[] CHANGES = ChangeLog.GetChangeLogs();
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
    
    public static CharSequence GetCvarText()
    {
        StringBuilder sb = new StringBuilder();
        final String endl = GetDialogMessageEndl();
        for(Map.Entry<String, KCVar.Group> item : KCVarSystem.CVars().entrySet())
        {
            KCVar.Group value = item.getValue();
            sb.append("------- ").append(value.name).append(" -------");
            sb.append(endl);
            for(KCVar cvar : value.list)
                sb.append(KCVarSystem.GenCVarString(cvar, endl));
            sb.append(endl);
        }
        return GetDialogMessage(sb.toString());
    }
    
	private TextHelper() {}
}

