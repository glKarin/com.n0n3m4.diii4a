package com.n0n3m4.q3e;

public final class Q3EGlobals
{
    public static final String CONST_PACKAGE_NAME = "com.karin.idTech4Amm";
    public static final String CONST_APP_NAME = "idTech4A++"; // "DIII4A++";

    // on-screen buttons index
    public static final int UI_JOYSTICK=0;
    public static final int UI_SHOOT=1;
    public static final int UI_JUMP=2;
    public static final int UI_CROUCH=3;
    public static final int UI_RELOADBAR=4;
    public static final int UI_PDA=5;
    public static final int UI_FLASHLIGHT=6;
    public static final int UI_SAVE=7;
    public static final int UI_1=8;
    public static final int UI_2=9;
    public static final int UI_3=10;
    public static final int UI_KBD=11;
    public static final int UI_CONSOLE = 12;
    public static final int UI_RUN = 13;
    public static final int UI_ZOOM = 14;
    public static final int UI_INTERACT = 15;
    public static final int UI_WEAPON_PANEL = 16;
    public static final int UI_SIZE=UI_WEAPON_PANEL+1;

    // on-screen item type
    public static final int TYPE_BUTTON = 0;
    public static final int TYPE_SLIDER = 1;
    public static final int TYPE_JOYSTICK = 2;
    public static final int TYPE_DISC = 3;
    public static final int TYPE_MOUSE = -1;

    // mouse
    public static final int MOUSE_EVENT = 1;
    public static final int MOUSE_DEVICE = 2;

    // on-screen button type
    public static final int ONSCREEN_BUTTON_TYPE_FULL = 0;
    public static final int ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM = 1;
    public static final int ONSCREEN_BUTTON_TYPE_CENTER = 2;
    public static final int ONSCREEN_BUTTON_TYPE_LEFT_TOP = 3;

    // on-screen button can hold
    public static final int ONSCRREN_BUTTON_NOT_HOLD = 0;
    public static final int ONSCRREN_BUTTON_CAN_HOLD = 1;

    // on-screen slider type
    public static final int ONSCRREN_SLIDER_STYLE_LEFT_RIGHT = 0;
    public static final int ONSCRREN_SLIDER_STYLE_DOWN_RIGHT = 1;

    // game state
    public static final int STATE_NONE = 0;
    public static final int STATE_ACT = 1; // RTCW4A-specific, keep
    public static final int STATE_GAME = 1 << 1; // map spawned
    public static final int STATE_KICK = 1 << 2; // RTCW4A-specific, keep
    public static final int STATE_LOADING = 1 << 3; // current GUI is guiLoading
    public static final int STATE_CONSOLE = 1 << 4; // fullscreen or not
    public static final int STATE_MENU = 1 << 5; // any menu excludes guiLoading
    public static final int STATE_DEMO = 1 << 6; // demo

    // game view control
    public static final int VIEW_MOTION_CONTROL_TOUCH = 1;
    public static final int VIEW_MOTION_CONTROL_GYROSCOPE = 1 << 1;
    public static final int VIEW_MOTION_CONTROL_ALL = VIEW_MOTION_CONTROL_TOUCH | VIEW_MOTION_CONTROL_GYROSCOPE;

    // game
    public static final String LIB_ENGINE_ID = "libdante.so"; // DOOM3
    public static final String LIB_ENGINE_RAVEN = "libdante_raven.so"; // Quake 4
    public static final String LIB_ENGINE_HUMANHEAD = "libdante_humanhead.so"; // Prey 2006

    public static final String CONFIG_FILE_DOOM3 = "DoomConfig.cfg"; // DOOM3
    public static final String CONFIG_FILE_QUAKE4 = "Quake4Config.cfg"; // Quake 4
    public static final String CONFIG_FILE_PREY = "preyconfig.cfg"; // Prey 2006

    public static final String GAME_DOOM3 = "doom3";
    public static final String GAME_QUAKE4 = "quake4";
    public static final String GAME_PREY = "prey(2006)";

    public static final String GAME_NAME_DOOM3 = "DOOM 3";
    public static final String GAME_NAME_QUAKE4 = "Quake 4";
    public static final String GAME_NAME_PREY = "Prey(2006)";

    public static final String GAME_BASE_DOOM3 = "base";
    public static final String GAME_BASE_QUAKE4 = "q4base";
    public static final String GAME_BASE_PREY = "preybase"; // Other platform is `base`

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
    public static final String[] PREY_LIBS = {
            "preygame",
    };

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

    public static final String[] CONTROLS_NAMES = {
            "Joystick",
            "Shoot",
            "Jump",
            "Crouch",
            "Reload",
            "PDA",
            "Flashlight",
            "Pause",
            "Extra 1",
            "Extra 2",
            "Extra 3",
            "Keyboard",
            "Console",
            "Run",
            "Zoom",
            "Interact",
            "Weapon"
    };

    private Q3EGlobals() {}
}
