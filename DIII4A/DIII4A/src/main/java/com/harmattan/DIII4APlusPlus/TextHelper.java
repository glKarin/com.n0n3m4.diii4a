package com.harmattan.DIII4APlusPlus;
import android.text.Html;
import android.content.Context;
import java.util.List;
import java.util.ArrayList;

public final class TextHelper
{
	public static final boolean USING_HTML = true;

	public static String GetDialogMessageEndl()
	{
		return USING_HTML ? "<br>" : "\n";
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
            for(String log : args)
                logs.add(log);
            return this;
        }

        public String GenString(String endl)
        {
            StringBuilder sb = new StringBuilder();
            sb.append("----- ").append(date).append(" (R").append(release).append(") -----");
            sb.append(endl);
            if(logs != null && !logs.isEmpty())
            {
                for(String str : logs)
                {
                    if(str != null)
                        sb.append("  * " + str);
                    sb.append(endl);
                }
            }
            return sb.toString();
        }

        public static ChangeLog Create()
        {
            return new ChangeLog();
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
        final String CHANGES[] = {
            Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")",
            Constants.CONST_NAME,
            "Update: " + ContextUtility.GetAppVersion(context) + (ContextUtility.BuildIsDebug(context) ? "(debug)" : ""),
            "Release: " + Constants.CONST_RELEASE + " (R" + Constants.CONST_UPDATE_RELEASE + ")",
            "Dev: " + GenLinkText("mailto:" + Constants.CONST_EMAIL, Constants.CONST_DEV),
            "Changes: ",
        };
        String endl = GetDialogMessageEndl();
        for(String str : CHANGES)
        {
            if(str != null)
                sb.append("  * " + str);
            sb.append(endl);
        }
        for(String str : Constants.CONST_CHANGES)
        {
            if(str != null)
                sb.append("  * " + str);
            sb.append(endl);
        }
		return GetDialogMessage(sb.toString());
	}

	public static CharSequence GetHelpText()
	{
        StringBuilder sb = new StringBuilder();
        final String CHANGES[] = {
            "On Android 11+, because of `Scoped-Storage`, must grant `Allow management of all files` permission.",
            null,
            "All special `CVAR`s are start with `harm_`.",
            null,
            "If game running crash(white screen): ",
            " 1. Make sure to allow `WRITE_EXTERNAL_STORAGE` permission.",
            " 2. Make sure `Game working directory` is right.",
            " 3. Uncheck 4th checkbox named `Use ETC1(or RGBA4444) cache` or clear ETC1 texture cache file manual on resource folder(exam: /sdcard/diii4a/(base/d3xp/d3le/cdoom/or...)/dds).",
            null,
            "If game is crash with flash-screen when playing a period of time: ",
            " 1. Out of graphics memory: `Clear vertex buffer` suggest to select 3rd or 2nd for clear vertex buffer every frame! If you select 1st, it will be same as original apk(ver 1.1.0 at 2013). It should work well on `Adreno` GPU of `Snapdragon`. More view in game, on DOOM3 console, cvar named `harm_r_clearVertexBuffer`.",
            null,
            "If want to load other mod: ",
            " 1. Input folder name of game mod to editor that under `User special` checkbox.",
            " 2. Check `User special` checkbox. `Commandline` will show `+set fs_game (the game mod)`.",
            " 3. If may want to choose game library, click `GameLib` and choose a game library. `Commandline` will show `+set harm_fs_gameLibPath (selected library path)`.",
        };
        String endl = GetDialogMessageEndl();
        for(String str : CHANGES)
        {
            if(str != null)
                sb.append("  * " + str);
            sb.append(endl);
        }
		return GetDialogMessage(sb.toString());
	}

	public static CharSequence GetAboutText(Context context)
	{
        StringBuilder sb = new StringBuilder();
        final String CHANGES[] = {
            Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")",
            Constants.CONST_NAME,
            "Changes by " + GenLinkText("https://forum.xda-developers.com/member.php?u=10584229", Constants.CONST_DEV)
            + "&lt;" + GenLinkText("mailto:" + Constants.CONST_EMAIL, Constants.CONST_EMAIL) + "&gt;",
            "Update: " + ContextUtility.GetAppVersion(context) + (ContextUtility.BuildIsDebug(context) ? "(debug)" : ""),
            "Release: " + Constants.CONST_RELEASE + " (R" + Constants.CONST_UPDATE_RELEASE + ")",
            null,
            "Source in `assets/source` folder in APK file. `doom3_droid.source.tgz` is DOOM3 source. `diii4a.source.tgz` is android frontend source.",
            "OpenGL shader source is in `assets/gl2progs.zip`.",
            "Or view in github `" + GenLinkText("https://github.com/glKarin/com.n0n3m4.diii4a", null) + "`, all changes on `" + GenLinkText("https://github.com/glKarin/com.n0n3m4.diii4a", "master") + "` branch.",
            null,
            "Special thanks: ",
            GenLinkText("https://4pda.ru/forum/index.php?showuser=7653620", "Sir Cat") + "@" + GenLinkText("https://4pda.ru/forum/index.php?showtopic=929753", "4PDA forum"),
        };
        String endl = GetDialogMessageEndl();
        for(String str : CHANGES)
        {
            if(str != null)
                sb.append("  * " + str);
            sb.append(endl);
        }
		return GetDialogMessage(sb.toString());
	}

    public static CharSequence GetChangesText()
    {
        final ChangeLog CHANGES[] = {
        ChangeLog.Create(Constants.CONST_RELEASE, Constants.CONST_UPDATE_RELEASE, Constants.CONST_CHANGES),

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
                             "Add texgen shader for like `D3XP` hell level sky.",
                             "Fix translucent object. i.e. window glass, transclucent Demon in `Classic DOOM` mod.",
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
        String endl = GetDialogMessageEndl();
        for(ChangeLog changeLog : CHANGES)
        {
            if(changeLog != null)
                sb.append(changeLog.GenString(endl));
            sb.append(endl);
        }
        return GetDialogMessage(sb.toString());
    }
    
	private TextHelper() {}
}

