package com.karin.idTech4Amm.sys;

import com.karin.idTech4Amm.BuildConfig;
import com.karin.idTech4Amm.lib.DateTimeUtility;
import com.karin.idTech4Amm.misc.TextHelper;

import java.util.Arrays;
import java.util.Date;

/**
 * Constants define
 */
public final class Constants
{
    public static final int    CONST_UPDATE_RELEASE = 56;
    public static final String CONST_RELEASE = "2024-08-31";
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan[DOOM3 20th Anniversary Edition]";
    public static final String CONST_APP_NAME = "idTech4A++"; // "DIII4A++";
    public static final String CONST_NAME = "DOOM III/Quake 4/Prey(2006)/DOOM 3 BFG for Android(Harmattan Edition)";
	public static final String CONST_MAIN_PAGE = "https://github.com/glKarin/com.n0n3m4.diii4a";
    public static final String CONST_TIEBA = "https://tieba.baidu.com/p/6825594793";
	public static final String CONST_DEVELOPER = "https://github.com/glKarin";
    public static final String CONST_DEVELOPER_XDA = "https://forum.xda-developers.com/member.php?u=10584229";
    public static final String CONST_PACKAGE = "com.karin.idTech4Amm";
    public static final String CONST_FDROID = "https://f-droid.org/packages/com.karin.idTech4Amm/";
	public static final String CONST_CHECK_FOR_UPDATE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/master/CHECK_FOR_UPDATE.json";
    public static final String CONST_LICENSE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/master/LICENSE";
	public static String[] CONST_CHANGES()
    {
        return new String[] {
            "Optimize PBR interaction lighting model in DOOM3/Quake4/Prey.",
            "Fix environment reflection shader in DOOM3/Quake4/Prey.",
            "Support switch weapon in DOOM 3(write `bind \"YOUR_KEY\" \"IMPULSE_51\"` to your DoomConfig.cfg or autoexec.cfg).",
            "Add ambient lighting model with `harm_r_lightingModel` 0 and remove r_noLight=2 in DOOM3/Quake4/Prey.",
            "Add `LibreCoop(RoE)` mod of DOOM3 support, game data directory named `librecoopxp`. More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/librecoop-dhewm3-coop", "LibreCoop") + "`.",
            "Add `Perfected Doom 3` mod of DOOM3 support, game data directory named `perfected`. More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/perfected-doom-3-version-500", "Perfected Doom 3") + "`.",
            "Add `Perfected Doom 3 : Resurrection of Evil` mod of DOOM3 support, game data directory named `perfected_roe`. More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/perfected-doom-3-version-500", "Perfected Doom 3 : Resurrection of Evil") + "`.",
        };
	};

    public static long GetBuildTimestamp()
    {
        return BuildConfig.BUILD_TIMESTAMP;
    }

    public static int GetBuildSDKVersion()
    {
        return BuildConfig.BUILD_SDK_VERSION;
    }

    public static boolean IsDebug()
    {
        return BuildConfig.DEBUG;
    }

    public static String GetBuildTime(String format)
    {
        return DateTimeUtility.Format(GetBuildTimestamp(), format);
    }
    
	private Constants() {}
}
