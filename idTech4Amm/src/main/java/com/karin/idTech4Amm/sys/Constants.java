package com.karin.idTech4Amm.sys;

import com.karin.idTech4Amm.BuildConfig;
import com.karin.idTech4Amm.lib.DateTimeUtility;
import com.karin.idTech4Amm.misc.TextHelper;

/**
 * Constants define
 */
public final class Constants
{
    public static final int    CONST_UPDATE_RELEASE = 62;
    public static final String CONST_RELEASE = "2025-03-11"; // 02-12
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
    public static final String CONST_CODE_ALIAS = "Lin Daiyu"; // Natasha Verena Caitlyn
	public static String[] CONST_CHANGES()
    {
        return new String[] {
            "Add `OpenJK` support, `" + TextHelper.GenLinkText("https://store.steampowered.com/app/6020/STAR_WARS_Jedi_Knight__Jedi_Academy/", "STAR WARS™ Jedi Knight - Jedi Academy™") + "` game standalone directory named `openja`, `" + TextHelper.GenLinkText("https://store.steampowered.com/app/6030/STAR_WARS_Jedi_Knight_II__Jedi_Outcast/", "STAR WARS™ Jedi Knight II - Jedi Outcast™") + "` game standalone directory named `openjo`. More view in `" + TextHelper.GenLinkText("https://github.com/JACoders/OpenJK", "OpenJK") + "`.",
            "Fix audio playing when disable OpenAL on Quake3/ETW/RTCW/RealRTCW.",
            "Add Vulkan renderer backend on Quake2.",
            "Fix create render context on GZDOOM, it will use OpenGL if Vulkan initialization fail.",
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
