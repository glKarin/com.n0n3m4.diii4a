package com.karin.idTech4Amm.sys;

import android.content.Context;

import com.karin.idTech4Amm.R;
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3ELang;

// game config
/*
 * config this can change launcher's game mod list.
 * If you want add a idTech4's mod and you have completed compiling c++ source code, you just edit this enum now.
 */
public enum Game
{
    // DOOM 3
    DOOM3_BASE(Q3EGameConstants.GAME_DOOM3, "base", "", "game", "", "base", false, R.string.doom_iii),
    DOOM3_D3XP(Q3EGameConstants.GAME_DOOM3, "d3xp", "d3xp", "d3xp", "", "d3xp", true, R.string.doom3_resurrection_of_evil_d3xp),

    DOOM3_CDOOM(Q3EGameConstants.GAME_DOOM3, "cdoom", "cdoom", "cdoom", "", "cdoom", true, R.string.classic_doom_cdoom),
    DOOM3_D3LE(Q3EGameConstants.GAME_DOOM3, "d3le", "d3le", "d3le", "d3xp", "d3le", true, R.string.doom3_bfg_the_lost_mission_d3le),
    DOOM3_RIVENSIN(Q3EGameConstants.GAME_DOOM3, "rivensin", "rivensin", "rivensin", "", "rivensin", true, R.string.rivensin_rivensin),
    DOOM3_HARDCORPS(Q3EGameConstants.GAME_DOOM3, "hardcorps", "hardcorps", "hardcorps", "", "hardcorps", true, R.string.hardcorps_hardcorps),
    DOOM3_OVERTHINKED(Q3EGameConstants.GAME_DOOM3, "overthinked", "overthinked", "overthinked", "", "overthinked", true, R.string.overthinked_doom_3),
    DOOM3_SABOT(Q3EGameConstants.GAME_DOOM3, "sabot", "sabot",  "sabot","d3xp", "sabot", true, R.string.stupid_angry_bot_a7x),
    DOOM3_HEXENEOC(Q3EGameConstants.GAME_DOOM3, "hexeneoc", "hexeneoc", "hexeneoc", "", "hexeneoc", true, R.string.hexen_edge_of_chaos),
    DOOM3_FRAGGINGFREE(Q3EGameConstants.GAME_DOOM3, "fraggingfree", "fraggingfree", "fraggingfree", "d3xp", "fraggingfree", true, R.string.fragging_free),
    DOOM3_LIBRECOOP(Q3EGameConstants.GAME_DOOM3, "librecoop", "librecoop", "librecoop", "", "librecoop", true, R.string.librecoop),
    DOOM3_LIBRECOOPXP(Q3EGameConstants.GAME_DOOM3, "librecoopxp", "librecoopxp", "librecoopxp", "d3xp", "librecoopxp", true, R.string.librecoop_roe),
    DOOM3_PERFECTED(Q3EGameConstants.GAME_DOOM3, "perfected", "perfected", "perfected",  "", "perfected", true, R.string.perfected_doom_3),
    DOOM3_PERFECTEDROE(Q3EGameConstants.GAME_DOOM3, "perfected_roe", "perfected_roe", "perfected_roe",  "d3xp", "perfected_roe", true, R.string.perfected_doom_3_resurrection_of_evil),
    DOOM3_PHOBOS(Q3EGameConstants.GAME_DOOM3, "tfphobos", "tfphobos", "tfphobos", "d3xp", "tfphobos", true, R.string.phobos),

    // Quake 4
    QUAKE4_BASE(Q3EGameConstants.GAME_QUAKE4, "q4base", "", "q4game", "", "q4base", false, R.string.quake_iv_q4base),
    QUAKE4_HARDQORE(Q3EGameConstants.GAME_QUAKE4, "hardqore", "hardqore", "hardqore",  "", "hardqore", true, R.string.hardqore),

    // Prey(2006)
    PREY_BASE(Q3EGameConstants.GAME_PREY, "preybase", "", "preygame", "", "preybase", false, R.string.prey_preybase),

    // Quake 1
    QUAKE1_BASE(Q3EGameConstants.GAME_QUAKE1, "id1", "", "idtech_quake", "", "darkplaces/id1", false, R.string.quake_1_base),

    // Quake 2
    QUAKE2_BASE(Q3EGameConstants.GAME_QUAKE2, "baseq2", "", "q2game", "", "baseq2", false, R.string.quake_2_base),
    QUAKE2_CTF(Q3EGameConstants.GAME_QUAKE2, "ctf", "ctf", "q2ctf", "", "ctf", true, R.string.quake_2_ctf),
    QUAKE2_ROGUE(Q3EGameConstants.GAME_QUAKE2, "rogue", "rogue", "q2rogue", "", "rogue", true, R.string.quake_2_rogue),
    QUAKE2_XATRIX(Q3EGameConstants.GAME_QUAKE2, "xatrix", "xatrix", "q2xatrix", "", "xatrix", true, R.string.quake_2_xatrix),
    QUAKE2_ZAERO(Q3EGameConstants.GAME_QUAKE2, "zaero", "zaero", "q2zaero", "", "zaero", true, R.string.quake_2_zaero),

    // Return to Castle Wolfenstein
    RTCW_BASE(Q3EGameConstants.GAME_RTCW, "main", "", "rtcwgame", "", "main", false, R.string.rtcw_base),

    // The dark mod
    TDM_BASE(Q3EGameConstants.GAME_TDM, "", "", "thedarkmod", "", "", false, R.string.tdm_base),

    // Quake 3
    QUAKE3_BASE(Q3EGameConstants.GAME_QUAKE3, "baseq3", "", "qagame", "", "baseq3", false, R.string.quake_3_base),
    QUAKE3_TEAMARENA(Q3EGameConstants.GAME_QUAKE3, "missionpack", "missionpack", "qagame_mp", "", "missionpack", true, R.string.quake_3_teamarena),

    // Doom3 BFG
    D3BFG_BASE(Q3EGameConstants.GAME_DOOM3BFG, "base", "", "RBDoom3BFG", "", "base", false, R.string.d3bfg_base),

    // GZDOOM
    //GZDOOM_BASE(Q3EGameConstants.GAME_GZDOOM, "", "", "", false, R.string.doom_base),
    GZDOOM_DOOM1(Q3EGameConstants.GAME_GZDOOM, "DOOM.WAD", "DOOM.WAD", "gzdoom", "", "DOOM.WAD", true, R.string.doom1_base),
    GZDOOM_DOOM2(Q3EGameConstants.GAME_GZDOOM, "DOOM2.WAD", "DOOM2.WAD", "gzdoom", "", "DOOM2.WAD", true, R.string.doom2_base),
    GZDOOM_FREEDOOM1(Q3EGameConstants.GAME_GZDOOM, "freedoom1.wad", "freedoom1.wad", "gzdoom", "", "freedoom1.wad", true, R.string.freedoom1_base),
    GZDOOM_FREEDOOM2(Q3EGameConstants.GAME_GZDOOM, "freedoom2.wad", "freedoom2.wad", "gzdoom", "", "freedoom2.wad", true, R.string.freedoom2_base),

    // Wolfenstein: Enemy Territory
    ETW_BASE(Q3EGameConstants.GAME_ETW, "etmain", "", "etwgame", "", "etmain", false, R.string.etw_base),

    // RealRTCW
    REALRTCW_BASE(Q3EGameConstants.GAME_REALRTCW, "Main", "", "realrtcwgame", "", "Main", false, R.string.realrtcw_base),

    // FTEQW
    FTEQW_Q1(Q3EGameConstants.GAME_FTEQW, "quake1", "quake1", "fteqw", "", "id1", true, R.string.quake_1_base),
    FTEQW_Q2(Q3EGameConstants.GAME_FTEQW, "quake2", "quake2", "fteqw", "", "baseq2", true, R.string.quake_2_base),
    FTEQW_Q3(Q3EGameConstants.GAME_FTEQW, "quake3", "quake3", "fteqw", "", "baseq3", true, R.string.quake_3_base),
    FTEQW_H2(Q3EGameConstants.GAME_FTEQW, "hexen2", "hexen2", "fteqw", "", "data1", true, R.string.hexen_2_base),
    FTEQW_HL(Q3EGameConstants.GAME_FTEQW, "halflife", "halflife", "fteqw", "", "valve", true, R.string.halflife_base),
    FTEQW_CS1_5(Q3EGameConstants.GAME_FTEQW, "cstrike_1_5", "halflife", "fteqw", "cstrike", "cstrike", true, R.string.cs_1_5_base),

    // OpenJA
    JA_BASE(Q3EGameConstants.GAME_JA, "base", "", "jagame", "", "base", false, R.string.openja_base),

    // OpenJO
    JO_BASE(Q3EGameConstants.GAME_JO, "base", "", "jospgame", "", "base", false, R.string.openjo_base),

    // SamTFE
    SAMTFE_BASE(Q3EGameConstants.GAME_SAMTFE, "", "", "samtfe", "", "", false, R.string.samtfe_base),

    // SamTSE
    SAMTSE_BASE(Q3EGameConstants.GAME_SAMTSE, "", "", "samtse", "", "", false, R.string.samtse_base),

    // Xash3D
    XASH3D(Q3EGameConstants.GAME_XASH3D, "valve", "valve", "xash3d", "", "", false, R.string.halflife_base),
    ;

    public final String  type; // game type: doom3/quake4/prey2006/......
    public final String  game; // game id: unique
    public final String  file; // game data folder/file name
    public final String  fs_game; // game command name
    public final String  lib; // game library file name, only for DOOM3/Quake4/Prey
    public final String  fs_game_base; // game mod data base folder name, e.g. d3xp
    public final boolean is_mod; // is a mod
    public final Object  name; // game name string resource's id or game name string

    Game(String type, String game, String fs_game, String lib, String fs_game_base, String file, boolean is_mod, Object name)
    {
        this.type = type;
        this.game = game;
        this.fs_game = fs_game;
        this.lib = lib;
        this.fs_game_base = fs_game_base;
        this.file = file;
        this.is_mod = is_mod;
        this.name = name;
    }

    public String GetName(Context context)
    {
        if(name instanceof Integer)
            return Q3ELang.tr(context, (Integer)name);
        else if(name instanceof String)
            return (String)name;
        else
            return "";
    }

    public static Game GetGameMod(String game, String mod)
    {
        for (Game value : values())
        {
            if(!value.type.equals(game))
                continue;
            if(!value.game.equals(mod))
                continue;
            return value;
        }
        return null;
    }
}
