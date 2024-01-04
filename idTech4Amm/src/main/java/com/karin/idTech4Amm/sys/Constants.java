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
    public static final int CONST_UPDATE_RELEASE = 36;
    public static final String CONST_RELEASE = "2023-12-31";
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
            "Fixed prelight shadow's shadow mapping.",
            "Fixed EFX Reverb in Quake4.",
            "Add translucent stencil shadow support in stencil shadow(bool cvar `harm_r_translucentStencilShadow`(default 0); float cvar `harm_r_stencilShadowAlpha` for setting transparency).",
            "Add float cvar `harm_ui_subtitlesTextScale` control subtitles's text scale in Prey.",
            "Support cvar `r_brightness`.",
            "Fixed weapon projectile's scorches decals rendering in Prey(2006).",
            "Add `Stupid Angry Bot`(a7x) mod of DOOM3 support(need DOOM3: RoE game data), game data directory named `sabot`. More view in`" + TextHelper.GenLinkText("https://www.moddb.com/downloads/sabot-alpha-7x", "SABot(a7x)") + "`.",
            "Add `Overthinked DooM^3` mod of DOOM3 support, game data directory named `overthinked`. More view in`" + TextHelper.GenLinkText("https://www.moddb.com/mods/overthinked-doom3", "Overthinked DooM^3") + "`.",
            "Add `Fragging Free` mod of DOOM3 support(need DOOM3: RoE game data), game data directory named `fraggingfree`. More view in`" + TextHelper.GenLinkText("https://www.moddb.com/mods/fragging-free", "Fragging Free") + "`.",
            "Add `HeXen:Edge of Chaos` mod of DOOM3 support, game data directory named `hexeneoc`. More view in`" + TextHelper.GenLinkText("https://www.moddb.com/mods/hexen-edge-of-chaos", "HexenEOC") + "`.",
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
