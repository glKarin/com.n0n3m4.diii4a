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
    public static final int CONST_UPDATE_RELEASE = 39;
    public static final String CONST_RELEASE = "2024-04-10";
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan";
    public static final String CONST_APP_NAME = "idTech4A++"; // "DIII4A++";
    public static final String CONST_NAME = "DOOM III/Quake 4/Prey(2006) for Android(Harmattan Edition)";
	public static final String CONST_MAIN_PAGE = "https://github.com/glKarin/com.n0n3m4.diii4a";
    public static final String CONST_TIEBA = "https://tieba.baidu.com/p/6825594793";
	public static final String CONST_DEVELOPER = "https://github.com/glKarin";
    public static final String CONST_DEVELOPER_XDA = "https://forum.xda-developers.com/member.php?u=10584229";
    public static final String CONST_PACKAGE = "com.karin.idTech4Amm";
    public static final String CONST_FDROID = "https://f-droid.org/packages/com.karin.idTech4Amm/";
	public static final String CONST_CHECK_FOR_UPDATE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/master/CHECK_FOR_UPDATE.json";
    public static final String CONST_LICENSE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/master/LICENSE";
	public static final String[] CONST_CHANGES = {
            "Support perforated surface shadow in shadow mapping.",
            "Add `LibreCoop` mod of DOOM3 support, game data directory named `librecoop`. More view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/librecoop-dhewm3-coop", "LibreCoop") + "`.",
            "Add `Quake II` support, game data directory named `baseq2`. More view in `" + TextHelper.GenLinkText("https://store.steampowered.com/app/2320/Quake_II/", "Quake II") + "`.",
            "Add `Quake III Arena` support, game data directory named `baseq3`; Add `Quake III Team Arena` support, game data directory named `missionpack`. More view in `" + TextHelper.GenLinkText("https://store.steampowered.com/app/2200/Quake_III_Arena/", "Quake III Arena") + "`.",
            "Add `Return to Castle Wolfenstein` support, game data directory named `main`. More view in `" + TextHelper.GenLinkText("https://www.moddb.com/games/return-to-castle-wolfenstein", "Return to Castle Wolfenstein") + "`.",
            "Add `The dark mod` support, game data directory named `darkmod`. More view in `" + TextHelper.GenLinkText("https://www.thedarkmod.com", "The Dark Mod") + "`.",
            "Add a on-screen button theme.",
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
