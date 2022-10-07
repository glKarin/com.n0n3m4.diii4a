package com.karin.idTech4Amm.sys;

import com.karin.idTech4Amm.misc.TextHelper;

import java.util.Arrays;

public final class Constants
{
    public static final String CONST_PREFERENCE_APP_CRASH_INFO = "_APP_CRASH_INFO";
    public static final String CONST_PREFERENCE_EXCEPTION_DEBUG = "_EXCEPTION_DEBUG";

    public static final int CONST_UPDATE_RELEASE = 13;
    public static final String CONST_RELEASE = "2022-10-23";
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan";
    public static final String CONST_APP_NAME = "idTech4A++"; // "DIII4A++";
    public static final String CONST_NAME = "DOOM III for Android(Harmattan Edition)";
	public static final String[] CONST_CHANGES = {
		"Fixup Strogg health station GUI interactive in `Quake 4`.",
		"Fixup skip cinematic in `Quake 4`.",
		"If `harm_g_alwaysRun` is 1, hold `Walk` key to walk in `Quake 4`.",
		"Fixup level map script fatal error or bug in `Quake 4`(All maps have not fatal errors no longer, but have some bugs yet.).",
		" `game/mcc_landing`: Player collision error on last elevator. You can jump before elevator ending or using `noclip`.",
		" `game/mcc_1`: Loading crash after last level ending. Using `map game/mcc_1` to reload.",
		" `game/convoy1`: State error is not care no longer and ignore. But sometimes has player collision error when jumping form vehicle, using `noclip`.",
		" `game/putra`: Script fatal error has fixed. But can not down on broken floor, using `noclip`.",
		" `game/waste`: Script fatal error has fixed.",
		//" `game/storage1 first`: Last end level elevator call GUI not work. Using `god` and jump into elevator, then run `trigger ontoElevatorTrig` command, and click trigger GUI for end level.",
		" `game/process1 first`: Last elevator has ins collision cause killing player. Using `god`. If tower's elevator GUI not work, using `teleport tgr_endlevel` to next level directly.",
		" `game/process1 second`: Second elevator has incorrect collision cause killing player(same as `game/process1 first` level). Using `god`.",
		" `game/core1`: Fixup first elevator platform not go up.",
		" `game/core2`: Fixup entity rotation.",
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
