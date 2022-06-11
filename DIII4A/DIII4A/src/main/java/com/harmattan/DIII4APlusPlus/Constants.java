package com.harmattan.DIII4APlusPlus;

public final class Constants
{
    public static final String CONST_PREFERENCE_APP_CRASH_INFO = "_APP_CRASH_INFO";

    public static final int CONST_UPDATE_RELEASE = 10;
    public static final String CONST_RELEASE = "2022-06-23";
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan";
    public static final String CONST_APP_NAME = "DIII4A++";
    public static final String CONST_NAME = "DOOM III for Android(Harmattan Edition)";
	public static final String[] CONST_CHANGES = {
        "Add `Rivensin` mod library support, game path name is `rivensin`, more view in `" + TextHelper.GenLinkText("https://www.moddb.com/mods/ruiner", null) + "`.",
		"The `Rivensin` game library support load DOOM3 base game map. But first must add include original DOOM3 all map script into `doom_main.script` of `Rivensin` mod file.",
        "Add weapon panel keys configure.",
		"Fix file access permission grant on Android 10.",
	};
	public static final String LIBS[] = {
		"game",
		"d3xp",
		"cdoom",
		"d3le",
		"rivensin",
	};

    public static final class PreferenceKey {
        public static final String LAUNCHER_ORIENTATION = "harm_launcher_orientation";
        public static final String RUN_BACKGROUND = "harm_run_background";
        public static final String HIDE_NAVIGATION_BAR = "harm_hide_nav";
        public static final String RENDER_MEM_STATUS = "harm_render_mem_status";
        public static final String MAP_BACK = "harm_map_back";
        public static final String WEAPON_PANEL_KEYS = "harm_weapon_panel_keys";
        
        private PreferenceKey() {}
    }
    
	private Constants() {}
}
