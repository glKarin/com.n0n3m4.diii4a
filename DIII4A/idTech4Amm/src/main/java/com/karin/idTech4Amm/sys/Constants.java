package com.karin.idTech4Amm.sys;

import java.util.Arrays;

public final class Constants
{
    public static final String CONST_PREFERENCE_APP_CRASH_INFO = "_APP_CRASH_INFO";
    public static final String CONST_PREFERENCE_EXCEPTION_DEBUG = "_EXCEPTION_DEBUG";

    public static final int CONST_UPDATE_RELEASE = 22;
    public static final String CONST_RELEASE = "2023-01-10";
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan";
    public static final String CONST_APP_NAME = "idTech4A++"; // "DIII4A++";
    public static final String CONST_NAME = "DOOM III/Quake 4/Prey(2006) for Android(Harmattan Edition)";
	public static final String CONST_MAIN_PAGE = "https://github.com/glKarin/com.n0n3m4.diii4a";
	public static final String CONST_DEVELOPER = "https://github.com/glKarin";
    public static final String CONST_DEVELOPER_XDA = "https://forum.xda-developers.com/member.php?u=10584229";
	public static final String CONST_CHECK_FOR_UPDATE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/package/CHECK_FOR_UPDATE.json";
	public static final String[] CONST_CHANGES = {
			"Support screen top edges with fullscreen.",
            "Add bad skybox render in Prey(2006).",
            "Add bad portal render in Prey(2006).",
            "Add `deathwalk` map append support in Prey(2006).",
	};

    public static final class PreferenceKey {
        public static final String LAUNCHER_ORIENTATION = "harm_launcher_orientation";
        public static final String RUN_BACKGROUND = "harm_run_background";
        public static final String HIDE_NAVIGATION_BAR = "harm_hide_nav";
        public static final String RENDER_MEM_STATUS = "harm_render_mem_status";
        public static final String MAP_BACK = "harm_map_back";
        public static final String WEAPON_PANEL_KEYS = "harm_weapon_panel_keys";
        public static final String ONSCREEN_BUTTON = "harm_onscreen_button";
        public static final String REDIRECT_OUTPUT_TO_FILE = "harm_redirect_output_to_file";
        public static final String NO_HANDLE_SIGNALS = "harm_no_handle_signals";
        public static final String VOLUME_UP_KEY = "harm_volume_up_key";
        public static final String VOLUME_DOWN_KEY = "harm_volume_down_key";
        public static final String HIDE_AD_BAR = "harm_hide_ad_bar";
		public static final String OPEN_QUAKE4_HELPER = "harm_open_quake4_helper";
		public static final String CONTROLS_CONFIG_POSITION_UNIT = "harm_controls_config_position_unit";
        public static final String COVER_EDGES = "harm_cover_edges";
        
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
