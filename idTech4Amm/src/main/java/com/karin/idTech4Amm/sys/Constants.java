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
    public static final String CONST_CODE_ALIAS = "Lin Daiyu"; // Natasha; Verena; Caitlyn
	public static String[] CONST_CHANGES()
    {
        return new String[] {
            "The Dark Mod update to version 2.13. More view in `" + TextHelper.GenLinkText("https://www.thedarkmod.com/posts/the-dark-mod-2-13-has-been-released/", "The Dark Mod(2.13)") + "`",
            "Add DOOM3-BFG new fonts and wide-character language support. Only support UTF-8(BOM) encoding translation file.",
            "Add Chinese translation patch on DOOM3. First extract `DOOM3 Chinese translation(by 3DM DOOM3-BFG Chinese patch)`, then add ` +set sys_lang chinese` to command line.",
            "Fix some The Dark Mod(ver 2.12) error.",
            "Add DOOM3-BFG occlusion culling with cvar `harm_r_occlusionCulling` on DOOM 3/Quake 4/Prey(2006).",
            "Fix some lights missing on ceil in map game/airdefense1 on Quake 4.",
            "Add game portal support on Prey(2006).",
            "Fix wrong resurrection position from deathwalk state when load game after restart application on Prey(2006).",
            "Support game data folder creation with `Game path tips` button on launcher `General` tab.",
            "[Warning]: RealRTCW(ver 5.0) and The Dark Mod(2.12) will be removed on next release in the future!",
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
