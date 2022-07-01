package com.harmattan.DIII4Amm;
import android.text.Html;
import android.content.Context;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Collection;

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
            for(String log : args)
                logs.add(log);
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
                        sb.append(FormatDialogMessageSpace("  * ") + str);
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
        String endl = GetDialogMessageEndl();
        for(String str : INFOS)
        {
            if(null != str)
                sb.append(str);
            sb.append(endl);
        }
        for(String str : Constants.CONST_CHANGES)
        {
            if(str != null)
                sb.append(FormatDialogMessageSpace("  * ") + str);
            sb.append(endl);
        }
		return GetDialogMessage(sb.toString());
	}

	public static CharSequence GetHelpText()
	{
        StringBuilder sb = new StringBuilder();
        final String[] HELPS = {
            "On Android 11+, because of `Scoped-Storage`, must grant `Allow management of all files` permission.",
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
			"  *. Some problems: ",
			"    1. After end game level and change next map, the player view can change to First-person, need input `pm_thirdPerson 1` on console to change back Third-person player view.",
        };
        String endl = GetDialogMessageEndl();
        for(String str : HELPS)
        {
            if(str != null)
			{
				if(str.startsWith(" "))
					sb.append(FormatDialogMessageHeaderSpace(str));
				else
					sb.append("* " + str);
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
        for(String str : ABOUTS)
        {
            if(str != null)
                sb.append(/*"  * " + */str);
            sb.append(endl);
        }
		return GetDialogMessage(sb.toString());
	}

    public static CharSequence GetChangesText()
    {
        final ChangeLog[] CHANGES = {
        ChangeLog.Create(Constants.CONST_RELEASE, Constants.CONST_UPDATE_RELEASE, Constants.CONST_CHANGES),

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
                    sb.append(FormatDialogMessageSpace("    ") + str.GenString(endl));
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
        };
        final Cvar[] GAME_CVARS = {
            Cvar.Create("harm_g_skipBerserkVision", "bool", "0", "Skip render berserk vision for power up."),
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
        
        Map<String, Cvar[]> cvarMap = new LinkedHashMap<>();
        cvarMap.put("Renderer", RENDSRER_CVARS);
        cvarMap.put("Common", COMMON_CVARS);
        cvarMap.put("Base game", GAME_CVARS);
        cvarMap.put("DOOM3 RoE", D3XP_CVARS);
        cvarMap.put("Classic DOOM", CDOOM_CVARS);
        cvarMap.put("The lost mission", D3LE_CVARS);
        cvarMap.put("Rivensin mod", RIVENSIN_CVARS);
        cvarMap.put("Hardcorps mod", HARDCORPS_CVARS);

        StringBuilder sb = new StringBuilder();
        String endl = GetDialogMessageEndl();
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

