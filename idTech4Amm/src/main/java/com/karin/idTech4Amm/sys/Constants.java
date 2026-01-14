package com.karin.idTech4Amm.sys;

import com.karin.idTech4Amm.BuildConfig;
import com.karin.idTech4Amm.lib.DateTimeUtility;
import com.karin.idTech4Amm.misc.TextHelper;

/**
 * Constants define
 */
public final class Constants
{
    public static final int    CONST_UPDATE_RELEASE = 70;
    public static final String CONST_RELEASE = "2025-12-21"; // 02-12; 05-08
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan";
    public static final String CONST_APP_NAME = "idTech4A++"; // "DIII4A++";
    public static final String CONST_NAME = "DOOM III/Quake 4/Prey(2006)/DOOM 3 BFG for Android(Harmattan Edition)";
	public static final String CONST_MAIN_PAGE = "https://github.com/glKarin/com.n0n3m4.diii4a";
    public static final String CONST_TIEBA = "https://tieba.baidu.com/p/6825594793";
	public static final String CONST_DEVELOPER = "https://github.com/glKarin";
    public static final String CONST_DEVELOPER_XDA = "https://forum.xda-developers.com/member.php?u=10584229";
    public static final String CONST_DISCORD = "https://discord.gg/KFshBra4kh";
    public static final String CONST_PACKAGE = "com.karin.idTech4Amm";
    public static final String CONST_FDROID = "https://f-droid.org/packages/com.karin.idTech4Amm/";
	public static final String CONST_CHECK_FOR_UPDATE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/master/CHECK_FOR_UPDATE.json";
    public static final String CONST_LICENSE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/master/LICENSE";
    public static final String CONST_TESTING_URL = "https://github.com/glKarin/com.n0n3m4.diii4a/releases/tag/android_testing";
    public static final String CONST_CODE_ALIAS = "Lin Daiyu"; // Natasha; Verena; Caitlyn; Lin Daiyu; Lu Yiping
	public static String[] CONST_CHANGES()
    {
        return new String[] {
                "Add game main thread stack size config on Menu > Option > Advance.",
                "Support edit on-screen buttons in gaming.",
				"Add `ECWolf`(ver 1.4.2) support, game standalone directory named `ecwolf`. More view in `" + TextHelper.GenLinkText("http://maniacsvault.net/ecwolf/", "ECWolf") + "`.",
                "Add `UZDoom`(ver 4.14.3) arm64 support, game standalone directory named `uzdoom`. More view in `" + TextHelper.GenLinkText("https://github.com/UZDoom/UZDoom", "UZDoom") + "`. And GZDoom is removed.",
                "Fix some GUIs in Quake 4/Prey(2006).",
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
