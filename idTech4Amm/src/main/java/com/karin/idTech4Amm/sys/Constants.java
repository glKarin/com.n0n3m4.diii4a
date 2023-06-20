package com.karin.idTech4Amm.sys;

import java.util.Arrays;

/**
 * Constants define
 */
public final class Constants
{
    public static final int CONST_UPDATE_RELEASE = 32;
    public static final String CONST_RELEASE = "2023-06-30";
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
	public static final String CONST_CHECK_FOR_UPDATE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/package/CHECK_FOR_UPDATE.json";
    public static final String CONST_LICENSE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/master/LICENSE";
	public static final String[] CONST_CHANGES = {
            "Add `zh` language.",
            "Move some on-screen settings to `Configure on-screen controls` page.",
            "DOOM3 add `full-body awareness` mod. Set bool cvar `harm_pm_fullBodyAwareness` to 1 enable, and using `harm_pm_fullBodyAwarenessOffset` setup offset.",
            "Support add external game library in `GameLib` at tab `General`(Testing. Not sure available for all device and Android version because of system security. You can compile own game mod library(armv7/armv8) with DIII4A project and run it using original idTech4A++).",
            "Support load external game library in `Game working directory`/`fs_game` folder instead of default game library of apk if enabled `Find game library in game data directory`(Testing. Not sure available for all device and Android version because of system security. You can compile own game mod library(armv7/armv8) with DIII4A project, and then named `gameaarch64.so` or `libgameaarch64.so` on arm64 device or named `gamearm.so` or `libgamearm.so` on arm32 device, and then put it on `Game working directory`/`fs_game` folder).",
            "Support jpg/png image texture file.",
	};

	// Launcher preference keys
    public static final class PreferenceKey {
        public static final String LAUNCHER_ORIENTATION = "harm_launcher_orientation";
        public static final String HIDE_AD_BAR = "harm_hide_ad_bar";
		//public static final String OPEN_QUAKE4_HELPER = "harm_open_quake4_helper";
        
        private PreferenceKey() {}
    }
    
	private Constants() {}
}
