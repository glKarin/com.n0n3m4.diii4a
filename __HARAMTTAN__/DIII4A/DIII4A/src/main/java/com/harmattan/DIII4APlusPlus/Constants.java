package com.harmattan.DIII4APlusPlus;

import java.util.List;
import java.util.ArrayList;

public final class Constants
{
    public static final String CONST_PREFERENCE_APP_CRASH_INFO = "_APP_CRASH_INFO";

    public static final int CONST_UPDATE_RELEASE = 7;
    public static final String CONST_RELEASE = "2022-05-05";
    public static final String CONST_EMAIL = "beyondk2000@gmail.com";
    public static final String CONST_DEV = "Karin";
    public static final String CONST_CODE = "Harmattan";
    public static final String CONST_APP_NAME = "DIII4A++";
    public static final String CONST_NAME = "DOOM III for Android(Harmattan Edition)";
	public static final String[] CONST_CHANGES = {
		"Fix shadow clipped.",
		"Fix sky box.",
        "Fix fog and blend light.",
        "Fix glass reflection.",
        "Add texgen shader for like `D3XP` hell level sky.",
        "Fix translucent object. i.e. window glass, transclucent Demon in `Classic DOOM` mod.",
        "Fix dynamic texture interaction. i.e. rotating fans.",
        "Fix `Berserk`, `Grabber`, `Helltime` vision effect(First set cvar `harm_g_skipBerserkVision`, `harm_g_skipWarpVision` and `harm_g_skipHelltimeVision` to 0).",
        "Fix screen capture image when quick save game or mission tips.",
        "Fix machine gun's ammo panel.",
        "Add light model setting with `Phong` and `Blinn-Phong` when render interaction shader pass(string cvar `harm_r_lightModel`).",
        "Add specular exponent setting in light model(float cvar `harm_r_specularExponent`).",
        "Default using program internal OpenGL shader.",
        "Reset extras virtual button size, and add Console(~) key.",
        "Add `Back` key function setting, add 3-Click to exit.",
	};
	public static final String LIBS[] = {
		"game",
		"d3xp",
		"cdoom",
		"d3le",
	};

    public static final class PreferenceKey {
        public static final String LAUNCHER_ORIENTATION = "harm_launcher_orientation";
        public static final String RUN_BACKGROUND = "harm_run_background";
        public static final String HIDE_NAVIGATION_BAR = "harm_hide_nav";
        public static final String RENDER_MEM_STATUS = "harm_render_mem_status";
        public static final String MAP_BACK = "harm_map_back";
        
        private PreferenceKey() {}
    }
    
	private Constants() {}
}
