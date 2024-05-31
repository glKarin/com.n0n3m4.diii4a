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
    public static final int CONST_UPDATE_RELEASE = 51;
    public static final String CONST_RELEASE = "2024-05-31";
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan";
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
            "Add `DOOM 3 BFG`(RBDOOM-3-BFG ver1.4.0) support, game data directory named `doom3bfg/base`. More view in `" + TextHelper.GenLinkText("https://github.com/RobertBeckebans/RBDOOM-3-BFG", "RBDOOM-3-BFG") + "` and `" + TextHelper.GenLinkText("https://store.steampowered.com/agecheck/app/208200/", "DOOM-3-BFG") + "`.",
            "Add `Quake I`(Darkplaces) support, game data directory named `darkplaces/id1`. More view in `" + TextHelper.GenLinkText("https://github.com/DarkPlacesEngine/darkplaces", "DarkPlaces") + "` and `" + TextHelper.GenLinkText("https://store.steampowered.com/app/2310/Quake/", "Quake I") + "`.",
            "Fix some shaders error on Mali GPU in The Dark Mod(v 2.12).",
            "Upgrade Quake2(Yamagi Quake II) version.",
            "Support debug render tools(exclude r_showSurfaceInfo) on multi-threading in DOOM3/Quake4/Prey(2006).",
            "Support switch lighting disabled in game with r_noLight 0 and 2 in DOOM3/Quake4/Prey(2006).",
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
