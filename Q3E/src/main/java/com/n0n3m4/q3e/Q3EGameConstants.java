package com.n0n3m4.q3e;

public final class Q3EGameConstants
{
    // game engine library
    public static final String LIB_ENGINE_ID        = "libidtech4.so"; // DOOM3
    public static final String LIB_ENGINE_RAVEN     = "libidtech4_raven.so"; // Quake 4
    public static final String LIB_ENGINE_HUMANHEAD = "libidtech4_humanhead.so"; // Prey 2006
    public static final String LIB_ENGINE2_ID       = "libyquake2.so"; // Quake 2
    public static final String LIB_ENGINE3_ID       = "libioquake3.so"; // Quake 3
    public static final String LIB_ENGINE3_RTCW     = "libiowolfsp.so"; // RTCW
    public static final String LIB_ENGINE4_TDM      = "libTheDarkMod.so"; // TDM
    public static final String LIB_ENGINE1_QUAKE    = "libdarkplaces.so"; // Quake 1
    public static final String LIB_ENGINE4_D3BFG    = "libRBDoom3BFG.so"; // Doom3-BFG
    public static final String LIB_ENGINE1_DOOM     = "libgzdoom.so"; // GZDOOM
    public static final String LIB_ENGINE3_ETW      = "libetl.so"; // ETW
    public static final String LIB_ENGINE3_REALRTCW = "libRealRTCW.so"; // RealRTCW
    public static final String LIB_ENGINE_FTEQW     = "libfteqw.so"; // FTEQW
    public static final String LIB_ENGINE3_JA       = "libopenjk_sp.so"; // Jedi Academy
    public static final String LIB_ENGINE3_JO       = "libopenjo_sp.so"; // Jedi Outcast
    public static final String LIB_ENGINE_SAMTFE    = "libSeriousSamTFE.so"; // Serious Sam First
    public static final String LIB_ENGINE_SAMTSE    = "libSeriousSamTSE.so"; // Serious Sam Second
    public static final String LIB_ENGINE_XASH3D    = "libxash3d.so"; // Xash3D
    public static final String LIB_ENGINE_SOURCE    = "libsource.so"; // Source Engine

    public static final String LIB_ENGINE4_D3BFG_VULKAN = "libRBDoom3BFGVulkan.so"; // Doom3-BFG(Vulkan)
    //public static final String LIB_ENGINE3_REALRTCW_5_0 = "libRealRTCW_5_0.so"; // RealRTCW(5.0)
    //public static final String LIB_ENGINE4_TDM_2_12     = "libTheDarkMod_2_12.so"; // TDM(2.12)


    // game engine version
    public static final String GAME_VERSION_CURRENT = null; // default current

    public static final String GAME_VERSION_D3BFG_OPENGL = "OpenGL"; // Doom3-BFG(OpenGL)
    public static final String GAME_VERSION_D3BFG_VULKAN = "Vulkan"; // Doom3-BFG(Vulkan)

    //public static final String GAME_VERSION_REALRTCW     = "5.1"; // RealRTCW 5.1

    //public static final String GAME_VERSION_TDM      = "2.13"; // TDM 2.13

    // game config file
    public static final String CONFIG_FILE_DOOM3    = "DoomConfig.cfg";
    public static final String CONFIG_FILE_QUAKE4   = "Quake4Config.cfg";
    public static final String CONFIG_FILE_PREY     = "preyconfig.cfg";
    public static final String CONFIG_FILE_QUAKE2   = "config.cfg";
    public static final String CONFIG_FILE_QUAKE3   = "q3config.cfg";
    public static final String CONFIG_FILE_RTCW     = "wolfconfig.cfg";
    public static final String CONFIG_FILE_TDM      = "Darkmod.cfg";
    public static final String CONFIG_FILE_QUAKE1   = "config.cfg";
    public static final String CONFIG_FILE_DOOM3BFG = "D3BFGConfig.cfg";
    public static final String CONFIG_FILE_GZDOOM   = "gzdoom.ini";
    public static final String CONFIG_FILE_ETW      = "etconfig.cfg";
    public static final String CONFIG_FILE_REALRTCW = "realrtcwconfig.cfg";
    public static final String CONFIG_FILE_FTEQW    = "fte.cfg";
    public static final String CONFIG_FILE_JA       = "openjk_sp.cfg";
    public static final String CONFIG_FILE_JO       = "openjo_sp.cfg";
    public static final String CONFIG_FILE_SAMTFE   = "";
    public static final String CONFIG_FILE_SAMTSE   = "";
    public static final String CONFIG_FILE_XASH3D   = "";
    public static final String CONFIG_FILE_SOURCE   = "cfg/config.cfg";

    // game type token
    public static final String GAME_DOOM3    = "doom3";
    public static final String GAME_QUAKE4   = "quake4";
    public static final String GAME_PREY     = "prey2006";
    public static final String GAME_QUAKE2   = "quake2";
    public static final String GAME_QUAKE3   = "quake3";
    public static final String GAME_RTCW     = "rtcw";
    public static final String GAME_TDM      = "tdm";
    public static final String GAME_QUAKE1   = "quake1";
    public static final String GAME_DOOM3BFG = "doom3bfg";
    public static final String GAME_GZDOOM   = "gzdoom";
    public static final String GAME_ETW      = "etw";
    public static final String GAME_REALRTCW = "realrtcw";
    public static final String GAME_FTEQW    = "fteqw";
    public static final String GAME_JA       = "openja";
    public static final String GAME_JO       = "openjo";
    public static final String GAME_SAMTFE   = "samtfe";
    public static final String GAME_SAMTSE   = "samtse";
    public static final String GAME_XASH3D   = "xash3d";
    public static final String GAME_SOURCE   = "source";

    // game name
    public static final String GAME_NAME_DOOM3    = "DOOM 3";
    public static final String GAME_NAME_QUAKE4   = "Quake 4";
    public static final String GAME_NAME_PREY     = "Prey(2006)";
    public static final String GAME_NAME_QUAKE2   = "Quake 2";
    public static final String GAME_NAME_QUAKE3   = "Quake 3";
    public static final String GAME_NAME_RTCW     = "RTCW"; // "Return to Castle Wolfenstein";
    public static final String GAME_NAME_TDM      = "Dark mod"; // The Dark Mod
    public static final String GAME_NAME_QUAKE1   = "Quake 1";
    public static final String GAME_NAME_DOOM3BFG = "DOOM 3 BFG";
    public static final String GAME_NAME_GZDOOM   = "GZDOOM";
    public static final String GAME_NAME_ETW      = "ETW"; // "Wolfenstein: Enemy Territory";
    public static final String GAME_NAME_REALRTCW = "RealRTCW";
    public static final String GAME_NAME_FTEQW    = "FTEQW";
    public static final String GAME_NAME_JA       = "Jedi Academy";
    public static final String GAME_NAME_JO       = "Jedi Outcast";
    public static final String GAME_NAME_SAMTFE   = "Serious Sam TFE";
    public static final String GAME_NAME_SAMTSE   = "Serious Sam TSE";
    public static final String GAME_NAME_XASH3D   = "Xash3D";
    public static final String GAME_NAME_SOURCE   = "Source Engine";

    // game base folder
    public static final String GAME_BASE_DOOM3      = "base";
    public static final String GAME_BASE_D3XP       = "d3xp";
    public static final String GAME_BASE_QUAKE4     = "q4base";
    public static final String GAME_BASE_PREY       = "preybase"; // Other platform is `base`
    public static final String GAME_BASE_QUAKE2     = "baseq2";
    public static final String GAME_BASE_QUAKE3     = "baseq3";
    public static final String GAME_BASE_RTCW       = "main";
    public static final String GAME_BASE_TDM        = ""; // the dark mod is standalone
    public static final String GAME_BASE_QUAKE1     = "darkplaces/id1"; // "darkplaces";
    public static final String GAME_BASE_QUAKE1_DIR = "id1";
    public static final String GAME_BASE_DOOM3BFG   = "base"; // RBDoom3BFG always in doom3bfg folder
    public static final String GAME_BASE_GZDOOM     = ""; // GZDOOM is standalone
    public static final String GAME_BASE_ETW        = "etmain";
    public static final String GAME_BASE_REALRTCW   = "Main";
    public static final String GAME_BASE_FTEQW      = "";
    public static final String GAME_BASE_JA         = "base";
    public static final String GAME_BASE_JO         = "base";
    public static final String GAME_BASE_SAMTFE     = "";
    public static final String GAME_BASE_SAMTSE     = "";
    public static final String GAME_BASE_XASH3D     = "valve";
    public static final String GAME_BASE_SOURCE     = "hl2";

    // game sub directory
    public static final String GAME_SUBDIR_DOOM3    = "doom3";
    public static final String GAME_SUBDIR_QUAKE4   = "quake4";
    public static final String GAME_SUBDIR_PREY     = "prey";
    public static final String GAME_SUBDIR_QUAKE2   = "quake2";
    public static final String GAME_SUBDIR_QUAKE3   = "quake3";
    public static final String GAME_SUBDIR_RTCW     = "rtcw";
    public static final String GAME_SUBDIR_TDM      = "darkmod";
    public static final String GAME_SUBDIR_QUAKE1   = "quake1";
    public static final String GAME_SUBDIR_DOOMBFG  = "doom3bfg";
    public static final String GAME_SUBDIR_GZDOOM   = "gzdoom";
    public static final String GAME_SUBDIR_ETW      = "etw";
    public static final String GAME_SUBDIR_REALRTCW = "realrtcw";
    public static final String GAME_SUBDIR_FTEQW    = "fteqw";
    public static final String GAME_SUBDIR_JA       = "openja";
    public static final String GAME_SUBDIR_JO       = "openjo";
    public static final String GAME_SUBDIR_SAMTFE   = "serioussamtfe";
    public static final String GAME_SUBDIR_SAMTSE   = "serioussamtse";
    public static final String GAME_SUBDIR_XASH3D   = "xash3d";
    public static final String GAME_SUBDIR_SOURCE   = "source";

    // game type index(ID)
    public static final int GAME_ID_DOOM3    = 0;
    public static final int GAME_ID_QUAKE4   = 1;
    public static final int GAME_ID_PREY     = 2;
    public static final int GAME_ID_RTCW     = 3;
    public static final int GAME_ID_QUAKE3   = 4;
    public static final int GAME_ID_QUAKE2   = 5;
    public static final int GAME_ID_QUAKE1   = 6;
    public static final int GAME_ID_DOOM3BFG = 7;
    public static final int GAME_ID_TDM      = 8;
    public static final int GAME_ID_GZDOOM   = 9;
    public static final int GAME_ID_ETW      = 10;
    public static final int GAME_ID_REALRTCW = 11;
    public static final int GAME_ID_FTEQW    = 12;
    public static final int GAME_ID_JA       = 13;
    public static final int GAME_ID_JO       = 14;
    public static final int GAME_ID_SAMTFE   = 15;
    public static final int GAME_ID_SAMTSE   = 16;
    public static final int GAME_ID_XASH3D   = 17;
    public static final int GAME_ID_SOURCE   = 18;

    public enum PatchResource
    {
        QUAKE4_SABOT, DOOM3_SABOT, DOOM3_RIVENSIN_ORIGIANL_LEVELS, DOOM3BFG_HLSL_SHADER, TDM_GLSL_SHADER, GZDOOM_RESOURCE, DOOM3_BFG_CHINESE_TRANSLATION, XASH3D_EXTRAS, XASH3D_CS16_EXTRAS, SOURCE_ENGINE_EXTRAS,
    }

/*
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
    };*/

    public static final String GAME_EXECUABLE = "game.arm";

    // extra internal game file version: <Game engine version>.<idTech4A++ patch version>
    public static final String TDM_GLSL_SHADER_VERSION        = "2.13.1"; // 1: init
    public static final String TDM_2_12_GLSL_SHADER_VERSION   = "2.12.6"; // 6: fix a integer to float convert
    public static final String RBDOOM3BFG_HLSL_SHADER_VERSION = "1.4.1"; // 1: init
    public static final String GZDOOM_VERSION                 = "4.14.1.1"; // 1: init
    public static final String XASH3D_VERSION                 = "0.21.1"; // 1: init
    public static final String SOURCE_ENGINE_VERSION          = "1.16.1"; // 1: init

    public static final int[]    GZDOOM_GL_VERSIONS       = {0, 330, 420, 430, 450,};
    public static final String[] QUAKE2_RENDERER_BACKENDS = {"gl1", "gles3", "vk",};

    public static final String[] XASH3D_REFS = {"gles1", "gl4es", "gles3compat", "soft",};
    public static final String[] XASH3D_SV_CLS = {"", "cs16", "cs16_yapb",};
    public static final String[] XASH3D_LIBS = {
            "libxash3d.so", "libxash3d_menu.so", "libxash3d_ref_gl4es.so", "libxash3d_ref_gles1.so", "libxash3d_ref_gles3compat.so", "libxash3d_ref_soft.so",
            "libcs16_client.so", "libcs16_menu.so", "libcs16_server.so", "libcs16_yapb.so",
            "libhlsdk_client.so", "libhlsdk_server.so",
            "libfilesystem_stdio.so",
    };

    public static final String[] SOURCE_ENGINE_SV_CLS = {"hl2", "cstrike", "portal", "dod", "episodic", "hl2mp", "hl1", "hl1mp",};

    private Q3EGameConstants()
    {
    }
}
