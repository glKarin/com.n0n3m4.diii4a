package com.n0n3m4.q3e;

public enum Q3EGame
{
    DOOM3(Q3EGameConstants.GAME_ID_DOOM3, Q3EGameConstants.GAME_DOOM3, Q3EGameConstants.LIB_ENGINE_ID, Q3EGameConstants.GAME_NAME_DOOM3, Q3EGameConstants.GAME_BASE_DOOM3,
            Q3EGameConstants.GAME_VERSION_DOOM3, Q3EGameConstants.GAME_SUBDIR_DOOM3, false, Q3EGameConstants.CONFIG_FILE_DOOM3,
            "fs_game", "fs_game_base", null, null,
            Q3EPreference.pref_harm_fs_game, Q3EPreference.pref_harm_user_mod, Q3EPreference.pref_harm_game_mod, Q3EPreference.pref_harm_game_lib, Q3EPreference.pref_params, Q3EPreference.pref_harm_command_record, null,
            Q3EKeyCodes.KeyCodesD3.class
    ),
    QUAKE4(Q3EGameConstants.GAME_ID_QUAKE4, Q3EGameConstants.GAME_QUAKE4, Q3EGameConstants.LIB_ENGINE_RAVEN, Q3EGameConstants.GAME_NAME_QUAKE4, Q3EGameConstants.GAME_BASE_QUAKE4,
            Q3EGameConstants.GAME_VERSION_QUAKE4, Q3EGameConstants.GAME_SUBDIR_QUAKE4, false, Q3EGameConstants.CONFIG_FILE_QUAKE4,
            "fs_game", "fs_game_base", null, null,
            Q3EPreference.pref_harm_q4_fs_game, Q3EPreference.pref_harm_q4_user_mod, Q3EPreference.pref_harm_q4_game_mod, Q3EPreference.pref_harm_q4_game_lib, Q3EPreference.pref_params_quake4, Q3EPreference.pref_harm_q4_command_record, null,
            Q3EKeyCodes.KeyCodesD3.class
    ),
    PREY(Q3EGameConstants.GAME_ID_PREY, Q3EGameConstants.GAME_PREY, Q3EGameConstants.LIB_ENGINE_HUMANHEAD, Q3EGameConstants.GAME_NAME_PREY, Q3EGameConstants.GAME_BASE_PREY,
            Q3EGameConstants.GAME_VERSION_PREY, Q3EGameConstants.GAME_SUBDIR_PREY, false, Q3EGameConstants.CONFIG_FILE_PREY,
            "fs_game", "fs_game_base", null, null,
            Q3EPreference.pref_harm_prey_fs_game, Q3EPreference.pref_harm_prey_user_mod, Q3EPreference.pref_harm_prey_game_mod, Q3EPreference.pref_harm_prey_game_lib, Q3EPreference.pref_params_prey, Q3EPreference.pref_harm_prey_command_record, null,
            Q3EKeyCodes.KeyCodesD3.class
    ),

    RTCW(Q3EGameConstants.GAME_ID_RTCW, Q3EGameConstants.GAME_RTCW, Q3EGameConstants.LIB_ENGINE3_RTCW, Q3EGameConstants.GAME_NAME_RTCW, Q3EGameConstants.GAME_BASE_RTCW,
            Q3EGameConstants.GAME_VERSION_RTCW, Q3EGameConstants.GAME_SUBDIR_RTCW, false, Q3EGameConstants.CONFIG_FILE_RTCW,
            "fs_game", null, null, ".wolf",
            Q3EPreference.pref_harm_rtcw_fs_game, Q3EPreference.pref_harm_rtcw_user_mod, Q3EPreference.pref_harm_rtcw_game_mod, Q3EPreference.pref_harm_rtcw_game_lib, Q3EPreference.pref_params_rtcw, Q3EPreference.pref_harm_rtcw_command_record, null,
            Q3EKeyCodes.KeyCodesRTCW.class
    ),
    QUAKE3(Q3EGameConstants.GAME_ID_QUAKE3, Q3EGameConstants.GAME_QUAKE3, Q3EGameConstants.LIB_ENGINE3_ID, Q3EGameConstants.GAME_NAME_QUAKE3, Q3EGameConstants.GAME_BASE_QUAKE3,
            Q3EGameConstants.GAME_VERSION_QUAKE3, Q3EGameConstants.GAME_SUBDIR_QUAKE3, false, Q3EGameConstants.CONFIG_FILE_QUAKE3,
            "fs_game", null, null, ".q3a",
            Q3EPreference.pref_harm_q3_fs_game, Q3EPreference.pref_harm_q3_user_mod, Q3EPreference.pref_harm_q3_game_mod, Q3EPreference.pref_harm_q3_game_lib, Q3EPreference.pref_params_q3, Q3EPreference.pref_harm_q3_command_record, null,
            Q3EKeyCodes.KeyCodesQ3.class
    ),

    QUAKE2(Q3EGameConstants.GAME_ID_QUAKE2, Q3EGameConstants.GAME_QUAKE2, Q3EGameConstants.LIB_ENGINE2_ID, Q3EGameConstants.GAME_NAME_QUAKE2, Q3EGameConstants.GAME_BASE_QUAKE2,
            Q3EGameConstants.GAME_VERSION_QUAKE2, Q3EGameConstants.GAME_SUBDIR_QUAKE2, false, Q3EGameConstants.CONFIG_FILE_QUAKE2,
            "game", null, null, ".yq2",
            Q3EPreference.pref_harm_q2_fs_game, Q3EPreference.pref_harm_q2_user_mod, Q3EPreference.pref_harm_q2_game_mod, Q3EPreference.pref_harm_q2_game_lib, Q3EPreference.pref_params_q2, Q3EPreference.pref_harm_q2_command_record, null,
            Q3EKeyCodes.KeyCodesQ3.class
    ),

    QUAKE1(Q3EGameConstants.GAME_ID_QUAKE1, Q3EGameConstants.GAME_QUAKE1, Q3EGameConstants.LIB_ENGINE1_QUAKE, Q3EGameConstants.GAME_NAME_QUAKE1, Q3EGameConstants.GAME_BASE_QUAKE1,
            Q3EGameConstants.GAME_VERSION_QUAKE1, Q3EGameConstants.GAME_SUBDIR_QUAKE1, false, Q3EGameConstants.CONFIG_FILE_QUAKE1,
            "game", null, "darkplaces", null,
            Q3EPreference.pref_harm_q1_fs_game, Q3EPreference.pref_harm_q1_user_mod, Q3EPreference.pref_harm_q1_game_mod, Q3EPreference.pref_harm_q1_game_lib, Q3EPreference.pref_params_q1, Q3EPreference.pref_harm_q1_command_record, null,
            Q3EKeyCodes.KeyCodesQ1.class
    ),

    DOOM3BFG(Q3EGameConstants.GAME_ID_DOOM3BFG, Q3EGameConstants.GAME_DOOM3BFG, Q3EGameConstants.LIB_ENGINE4_D3BFG, Q3EGameConstants.GAME_NAME_DOOM3BFG, Q3EGameConstants.GAME_BASE_DOOM3BFG,
            Q3EGameConstants.GAME_VERSION_DOOM3BFG, Q3EGameConstants.GAME_SUBDIR_DOOMBFG, false, Q3EGameConstants.CONFIG_FILE_DOOM3BFG,
            "fs_game", "fs_game_base", null, ".local/share/rbdoom3bfg",
            Q3EPreference.pref_harm_d3bfg_fs_game, Q3EPreference.pref_harm_d3bfg_user_mod, Q3EPreference.pref_harm_d3bfg_game_mod, Q3EPreference.pref_harm_d3bfg_game_lib, Q3EPreference.pref_params_d3bfg, Q3EPreference.pref_harm_d3bfg_command_record, Q3EPreference.pref_harm_d3bfg_rendererBackend,
            Q3EKeyCodes.KeyCodesD3BFG.class
    ),

    TDM(Q3EGameConstants.GAME_ID_TDM, Q3EGameConstants.GAME_TDM, Q3EGameConstants.LIB_ENGINE4_TDM, Q3EGameConstants.GAME_NAME_TDM, Q3EGameConstants.GAME_BASE_TDM,
            Q3EGameConstants.GAME_VERSION_TDM, Q3EGameConstants.GAME_SUBDIR_TDM, true, Q3EGameConstants.CONFIG_FILE_TDM,
            "fs_currentfm" /* fs_mod */, null, "fms", null,
            Q3EPreference.pref_harm_tdm_fs_game, Q3EPreference.pref_harm_tdm_user_mod, Q3EPreference.pref_harm_tdm_game_mod, Q3EPreference.pref_harm_tdm_game_lib, Q3EPreference.pref_params_tdm, Q3EPreference.pref_harm_tdm_command_record, null /* Q3EPreference.pref_harm_tdm_version */,
            Q3EKeyCodes.KeyCodesD3.class
    ),

    GZDOOM(Q3EGameConstants.GAME_ID_GZDOOM, Q3EGameConstants.GAME_GZDOOM, Q3EGameConstants.LIB_ENGINE1_DOOM, Q3EGameConstants.GAME_NAME_GZDOOM, Q3EGameConstants.GAME_BASE_GZDOOM,
            Q3EGameConstants.GAME_VERSION_GZDOOM, Q3EGameConstants.GAME_SUBDIR_GZDOOM, true, Q3EGameConstants.CONFIG_FILE_GZDOOM,
            "iwad", null, null, ".config/gzdoom",
            Q3EPreference.pref_harm_gzdoom_fs_game, Q3EPreference.pref_harm_gzdoom_user_mod, Q3EPreference.pref_harm_gzdoom_game_mod, Q3EPreference.pref_harm_gzdoom_game_lib, Q3EPreference.pref_params_gzdoom, Q3EPreference.pref_harm_gzdoom_command_record, null,
            Q3EKeyCodes.KeyCodesSDL.class
    ),

    ETW(Q3EGameConstants.GAME_ID_ETW, Q3EGameConstants.GAME_ETW, Q3EGameConstants.LIB_ENGINE3_ETW, Q3EGameConstants.GAME_NAME_ETW, Q3EGameConstants.GAME_BASE_ETW,
            Q3EGameConstants.GAME_VERSION_ETW, Q3EGameConstants.GAME_SUBDIR_ETW, false, Q3EGameConstants.CONFIG_FILE_ETW,
            "fs_game", null, null, ".etlegacy/legacy",
            Q3EPreference.pref_harm_etw_fs_game, Q3EPreference.pref_harm_etw_user_mod, Q3EPreference.pref_harm_etw_game_mod, Q3EPreference.pref_harm_etw_game_lib, Q3EPreference.pref_params_etw, Q3EPreference.pref_harm_etw_command_record, null,
            Q3EKeyCodes.KeyCodesQ3.class
    ),

    REALRTCW(Q3EGameConstants.GAME_ID_REALRTCW, Q3EGameConstants.GAME_REALRTCW, Q3EGameConstants.LIB_ENGINE3_REALRTCW, Q3EGameConstants.GAME_NAME_REALRTCW, Q3EGameConstants.GAME_BASE_REALRTCW,
            Q3EGameConstants.GAME_VERSION_REALRTCW, Q3EGameConstants.GAME_SUBDIR_REALRTCW, false, Q3EGameConstants.CONFIG_FILE_REALRTCW,
            "fs_game", null, null, ".realrtcw",
            Q3EPreference.pref_harm_realrtcw_fs_game, Q3EPreference.pref_harm_realrtcw_user_mod, Q3EPreference.pref_harm_realrtcw_game_mod, Q3EPreference.pref_harm_realrtcw_game_lib, Q3EPreference.pref_params_realrtcw, Q3EPreference.pref_harm_realrtcw_command_record, null /* Q3EPreference.pref_harm_realrtcw_version */,
            Q3EKeyCodes.KeyCodesRTCW.class
    ),

    FTEQW(Q3EGameConstants.GAME_ID_FTEQW, Q3EGameConstants.GAME_FTEQW, Q3EGameConstants.LIB_ENGINE_FTEQW, Q3EGameConstants.GAME_NAME_FTEQW, Q3EGameConstants.GAME_BASE_FTEQW,
            Q3EGameConstants.GAME_VERSION_FTEQW, Q3EGameConstants.GAME_SUBDIR_FTEQW, true, Q3EGameConstants.CONFIG_FILE_FTEQW,
            "" /* game */, "game", null, null,
            Q3EPreference.pref_harm_fteqw_fs_game, Q3EPreference.pref_harm_fteqw_user_mod, Q3EPreference.pref_harm_fteqw_game_mod, Q3EPreference.pref_harm_fteqw_game_lib, Q3EPreference.pref_params_fteqw, Q3EPreference.pref_harm_fteqw_command_record, null,
            Q3EKeyCodes.KeyCodesQ3.class
    ),

    JA(Q3EGameConstants.GAME_ID_JA, Q3EGameConstants.GAME_JA, Q3EGameConstants.LIB_ENGINE3_JA, Q3EGameConstants.GAME_NAME_JA, Q3EGameConstants.GAME_BASE_JA,
            Q3EGameConstants.GAME_VERSION_JA, Q3EGameConstants.GAME_SUBDIR_JA, false, Q3EGameConstants.CONFIG_FILE_JA,
            "fs_game", null, null, null,
            Q3EPreference.pref_harm_ja_fs_game, Q3EPreference.pref_harm_ja_user_mod, Q3EPreference.pref_harm_ja_game_mod, Q3EPreference.pref_harm_ja_game_lib, Q3EPreference.pref_params_ja, Q3EPreference.pref_harm_ja_command_record, null,
            Q3EKeyCodes.KeyCodesJK.class
    ),
    JO(Q3EGameConstants.GAME_ID_JO, Q3EGameConstants.GAME_JO, Q3EGameConstants.LIB_ENGINE3_JO, Q3EGameConstants.GAME_NAME_JO, Q3EGameConstants.GAME_BASE_JO,
            Q3EGameConstants.GAME_VERSION_JO, Q3EGameConstants.GAME_SUBDIR_JO, false, Q3EGameConstants.CONFIG_FILE_JO,
            "fs_game", null, null, null,
            Q3EPreference.pref_harm_jo_fs_game, Q3EPreference.pref_harm_jo_user_mod, Q3EPreference.pref_harm_jo_game_mod, Q3EPreference.pref_harm_jo_game_lib, Q3EPreference.pref_params_jo, Q3EPreference.pref_harm_jo_command_record, null,
            Q3EKeyCodes.KeyCodesJK.class
    ),

    SAMTFE(Q3EGameConstants.GAME_ID_SAMTFE, Q3EGameConstants.GAME_SAMTFE, Q3EGameConstants.LIB_ENGINE_SAMTFE, Q3EGameConstants.GAME_NAME_SAMTFE, Q3EGameConstants.GAME_BASE_SAMTFE,
            Q3EGameConstants.GAME_VERSION_SAMTFE, Q3EGameConstants.GAME_SUBDIR_SAMTFE, true, Q3EGameConstants.CONFIG_FILE_SAMTFE,
            "", null, null, null,
            Q3EPreference.pref_harm_samtfe_fs_game, Q3EPreference.pref_harm_samtfe_user_mod, Q3EPreference.pref_harm_samtfe_game_mod, Q3EPreference.pref_harm_samtfe_game_lib, Q3EPreference.pref_params_samtfe, Q3EPreference.pref_harm_samtfe_command_record, null,
            Q3EKeyCodes.KeyCodesSDL.class
    ),
    SAMTSE(Q3EGameConstants.GAME_ID_SAMTSE, Q3EGameConstants.GAME_SAMTSE, Q3EGameConstants.LIB_ENGINE_SAMTSE, Q3EGameConstants.GAME_NAME_SAMTSE, Q3EGameConstants.GAME_BASE_SAMTSE,
            Q3EGameConstants.GAME_VERSION_SAMTSE, Q3EGameConstants.GAME_SUBDIR_SAMTSE, true, Q3EGameConstants.CONFIG_FILE_SAMTSE,
            "", null, null, null,
            Q3EPreference.pref_harm_samtse_fs_game, Q3EPreference.pref_harm_samtse_user_mod, Q3EPreference.pref_harm_samtse_game_mod, Q3EPreference.pref_harm_samtse_game_lib, Q3EPreference.pref_params_samtse, Q3EPreference.pref_harm_samtse_command_record, null,
            Q3EKeyCodes.KeyCodesSDL.class
    ),

    XASH3D(Q3EGameConstants.GAME_ID_XASH3D, Q3EGameConstants.GAME_XASH3D, Q3EGameConstants.LIB_ENGINE_XASH3D, Q3EGameConstants.GAME_NAME_XASH3D, Q3EGameConstants.GAME_BASE_XASH3D,
            Q3EGameConstants.GAME_VERSION_XASH3D, Q3EGameConstants.GAME_SUBDIR_XASH3D, true, Q3EGameConstants.CONFIG_FILE_XASH3D,
            "game", null, null, null,
            Q3EPreference.pref_harm_xash3d_fs_game, Q3EPreference.pref_harm_xash3d_user_mod, Q3EPreference.pref_harm_xash3d_game_mod, Q3EPreference.pref_harm_xash3d_game_lib, Q3EPreference.pref_params_xash3d, Q3EPreference.pref_harm_xash3d_command_record, null,
            Q3EKeyCodes.KeyCodesAndroid.class
    ),

    SOURCE(Q3EGameConstants.GAME_ID_SOURCE, Q3EGameConstants.GAME_SOURCE, Q3EGameConstants.LIB_ENGINE_SOURCE, Q3EGameConstants.GAME_NAME_SOURCE, Q3EGameConstants.GAME_BASE_SOURCE,
            Q3EGameConstants.GAME_VERSION_SOURCE, Q3EGameConstants.GAME_SUBDIR_SOURCE, true, Q3EGameConstants.CONFIG_FILE_SOURCE,
            "game", null, null, null,
            Q3EPreference.pref_harm_source_fs_game, Q3EPreference.pref_harm_source_user_mod, Q3EPreference.pref_harm_source_game_mod, Q3EPreference.pref_harm_source_game_lib, Q3EPreference.pref_params_source, Q3EPreference.pref_harm_source_command_record, null,
            Q3EKeyCodes.KeyCodesAndroid.class
    ),
    ;

    public final int    ID;
    public final String TYPE;

    public final String ENGINE_LIB;
    public final String NAME;
    public final String BASE;
    public final String VERSION;
    public final String DIR;

    public final boolean STANDALONE;
    public final String  CONFIG_FILE;

    public final String MOD_PARM;
    public final String MOD_SECONDARY_PARM;
    public final String MOD_DIR;
    public final String HOME_DIR;

    public final String   PREF_MOD;
    public final String   PREF_MOD_ENABLED;
    public final String   PREF_MOD_USER;
    public final String   PREF_MOD_LIB;
    public final String   PREF_COMMAND;
    public final String   PREF_CMD_RECORD;
    public final String   PREF_VERSION;
    public final Class<?> KEYCODE;

    Q3EGame(int ID, String TYPE, String ENGINE_LIB, String NAME, String BASE, String VERSION, String DIR, boolean STANDALONE, String CONFIG_FILE, String MOD_PARM, String MOD_SECONDARY_PARM, String MOD_DIR, String HOME_DIR, String PREF_MOD, String PREF_MOD_ENABLED, String PREF_MOD_USER, String PREF_MOD_LIB, String PREF_COMMAND, String PREF_CMD_RECORD, String PREF_VERSION, Class<?> KEYCODE)
    {
        this.ID = ID;
        this.TYPE = TYPE;
        this.ENGINE_LIB = ENGINE_LIB;
        this.NAME = NAME;
        this.BASE = BASE;
        this.VERSION = VERSION;
        this.DIR = DIR;
        this.STANDALONE = STANDALONE;
        this.CONFIG_FILE = CONFIG_FILE;
        this.MOD_PARM = MOD_PARM;
        this.MOD_SECONDARY_PARM = MOD_SECONDARY_PARM;
        this.MOD_DIR = MOD_DIR;
        this.HOME_DIR = HOME_DIR;
        this.PREF_MOD = PREF_MOD;
        this.PREF_MOD_ENABLED = PREF_MOD_ENABLED;
        this.PREF_MOD_USER = PREF_MOD_USER;
        this.PREF_MOD_LIB = PREF_MOD_LIB;
        this.PREF_COMMAND = PREF_COMMAND;
        this.PREF_CMD_RECORD = PREF_CMD_RECORD;
        this.PREF_VERSION = PREF_VERSION;
        this.KEYCODE = KEYCODE;
    }

    public static Q3EGame Find(int index)
    {
        Q3EGame[] values = values();
        if(index >= 0 && index < values.length)
            return values[index];
        else
            return values[Q3EGameConstants.GAME_ID_DOOM3]; // DOOM3
    }

    public static Q3EGame Find(String type)
    {
        Q3EGame[] values = values();
        for(Q3EGame value : values)
        {
            if(value.TYPE.equalsIgnoreCase(type))
                return value;
        }
        return values[Q3EGameConstants.GAME_ID_DOOM3]; // DOOM3
    }

    public static Q3EGame FindOrNull(int index)
    {
        Q3EGame[] values = values();
        if(index >= 0 && index < values.length)
            return values[index];
        else
            return null;
    }

    public static Q3EGame FindOrNull(String type)
    {
        Q3EGame[] values = values();
        for(Q3EGame value : values)
        {
            if(value.TYPE.equalsIgnoreCase(type))
                return value;
        }
        return null;
    }
}
