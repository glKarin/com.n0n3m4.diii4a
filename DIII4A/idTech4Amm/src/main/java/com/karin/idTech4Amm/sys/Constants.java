package com.karin.idTech4Amm.sys;

import com.karin.idTech4Amm.misc.TextHelper;

import java.util.Arrays;

public final class Constants
{
    public static final String CONST_PREFERENCE_APP_CRASH_INFO = "_APP_CRASH_INFO";
    public static final String CONST_PREFERENCE_EXCEPTION_DEBUG = "_EXCEPTION_DEBUG";

    public static final int CONST_UPDATE_RELEASE = 19;
    public static final String CONST_RELEASE = "2022-11-16";
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan";
    public static final String CONST_APP_NAME = "idTech4A++"; // "DIII4A++";
    public static final String CONST_NAME = "DOOM III/Quake IV for Android(Harmattan Edition)";
	public static final String CONST_MAIN_PAGE = "https://github.com/glKarin/com.n0n3m4.diii4a";
	public static final String CONST_CHECK_FOR_UPDATE_URL = "https://raw.githubusercontent.com/glKarin/com.n0n3m4.diii4a/package/CHECK_FOR_UPDATE.json";
	public static final String[] CONST_CHANGES = {
			"Fixup middle bridge door GUI not interactive of level `game/tram1` in Quake 4.",
			"Fixup elevator 1 with a monster GUI not interactive of level `game/process2` in Quake 4.",
	};
	public static final String[] LIBS = {
		"game",
		"d3xp",
		"cdoom",
		"d3le",
		"rivensin",
        "hardcorps",
	};
    public static final String[] Q4_LIBS = {
        "q4game",
	};
    public static final String GAME_DOOM3 = "doom3";
    public static final String GAME_QUAKE4 = "quake4";
    
    public static final String[] QUAKE4_MAPS = {
		"airdefense1",
		"airdefense2",
		"hangar1",
		"hangar2",
		"mcc_landing",
		"mcc_1",
		"convoy1",
		"building_b",
		"convoy2",
		"convoy2b",
		"hub1",
		"hub2",
		"medlabs",
		"walker",
		"dispersal",
		"recomp",
		"putra",
		"waste",
		"mcc_2",
		"storage1 first",
		"storage2",
		"storage1 second",
		"tram1",
		"tram1b",
		"process1 first",
		"process2",
		"process1 second",
		"network1",
		"network2",
		"core1",
		"core2",
	};

    public static final String[] QUAKE4_LEVELS = {
		"AIR DEFENSE BUNKER", // Act I
		"AIR DEFENSE TRENCHES",
		"HANGAR PERIMETER",
		"INTERIOR HANGAR",
		"MCC LANDING SITE",
		"OPERATION: ADVANTAGE", // Act II
		"CANYON",
		"PERIMETER DEFENSE STATION",
		"AQUEDUCTS",
		"AQUEDUCTS ANNEX",
		"NEXUS HUB TUNNELS",
		"NEXUS HUB",
		"STROGG MEDICAL FACILITIES", // Act III
		"CONSTRUCTION ZONE",
		"DISPERSAL FACILITY",
		"RECOMPOSITION CENTER",
		"PUTRIFICATION CENTER",
		"WASTE PROCESSING FACILITY",
		"OPERATION: LAST HOPE", // Act IV
		"DATA STORAGE TERMINAL",
		"DATA STORAGE SECURITY",
		"DATA STORAGE TERMINAL",
		"TRAM HUB STATION",
		"TRAM RAIL",
		"DATA PROCESSING TERMINAL",
		"DATA PROCESSING SECURITY",
		"DATA PROCESSING TERMINAL",
		"DATA NETWORKING TERMINAL",
		"DATA NETWORKING SECURITY",
		"NEXUS CORE", // Act V
		"THE NEXUS",
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
