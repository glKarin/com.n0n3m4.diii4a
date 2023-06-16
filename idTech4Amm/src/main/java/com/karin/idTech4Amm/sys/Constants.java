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
	public static final String[] CONST_CHANGES = {
            "Add `zh` language.",
            "Move some on-screen settings to `Configure on-screen controls` page.",
	};

	// Launcher preference keys
    public static final class PreferenceKey {
        public static final String LAUNCHER_ORIENTATION = "harm_launcher_orientation";
        public static final String HIDE_AD_BAR = "harm_hide_ad_bar";
		//public static final String OPEN_QUAKE4_HELPER = "harm_open_quake4_helper";
        
        private PreferenceKey() {}
    }

    private static int[] _defaultArgs;
    private static int[] _defaultType;
    public static void DumpDefaultOnScreenConfig(int[] args, int[] type)
    {
        _defaultArgs = Arrays.copyOf(args, args.length);
        _defaultType = Arrays.copyOf(type, args.length);
    }

    public static void RestoreDefaultOnScreenConfig(int[] args, int[] type)
    {
        System.arraycopy(_defaultArgs, 0, args, 0, args.length);
        System.arraycopy(_defaultType, 0, type, 0, type.length);
    }
    
	private Constants() {}
}
