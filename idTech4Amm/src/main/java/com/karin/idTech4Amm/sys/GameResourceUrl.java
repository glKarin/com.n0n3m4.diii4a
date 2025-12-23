package com.karin.idTech4Amm.sys;

import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.karin.KStr;

import java.util.ArrayList;
import java.util.List;

public enum GameResourceUrl
{
    // KARIN_NEW_GAME_BOOKMARK

    // DOOM 3
    DOOM3_BASE(Q3EGameConstants.GAME_DOOM3, "base", "", "https://store.steampowered.com/app/208200/DOOM_3/", 2),
    DOOM3_D3XP(Q3EGameConstants.GAME_DOOM3, "d3xp", "", "https://store.steampowered.com/app/9070/DOOM_3_Resurrection_of_Evil/", 2),
    DOOM3_CDOOM(Q3EGameConstants.GAME_DOOM3, "cdoom", "", "https://www.moddb.com/mods/classic-doom-3", 3),
    DOOM3_D3LE(Q3EGameConstants.GAME_DOOM3, "d3le", "", "https://www.moddb.com/mods/the-lost-mission", 3),
    DOOM3_RIVENSIN(Q3EGameConstants.GAME_DOOM3, "rivensin", "", "https://www.moddb.com/mods/ruiner", 3),
    DOOM3_HARDCORPS(Q3EGameConstants.GAME_DOOM3, "hardcorps", "", "https://www.moddb.com/mods/hardcorps", 3),
    DOOM3_OVERTHINKED(Q3EGameConstants.GAME_DOOM3, "overthinked", "", "https://www.moddb.com/mods/overthinked-doom3", 3),
    DOOM3_SABOT(Q3EGameConstants.GAME_DOOM3, "sabot", "", "https://www.moddb.com/games/doom-3-resurrection-of-evil/downloads/sabot-alpha-7x", 3),
    DOOM3_HEXENEOC(Q3EGameConstants.GAME_DOOM3, "hexeneoc", "", "https://www.moddb.com/mods/hexen-edge-of-chaos", 3),
    DOOM3_FRAGGINGFREE(Q3EGameConstants.GAME_DOOM3, "fraggingfree", "", "https://www.moddb.com/mods/fragging-free", 3),
    DOOM3_LIBRECOOP(Q3EGameConstants.GAME_DOOM3, "librecoop", "", "https://www.moddb.com/mods/librecoop-dhewm3-coop", 3),
    DOOM3_LIBRECOOPXP(Q3EGameConstants.GAME_DOOM3, "librecoopxp", "", "https://www.moddb.com/mods/librecoop-dhewm3-coop", 3),
    DOOM3_PERFECTED(Q3EGameConstants.GAME_DOOM3, "perfected", "", "https://www.moddb.com/mods/perfected-doom-3-version-500", 3),
    DOOM3_PERFECTEDROE(Q3EGameConstants.GAME_DOOM3, "perfected_roe", "", "https://www.moddb.com/mods/perfected-doom-3-version-500", 3),
    DOOM3_PHOBOS(Q3EGameConstants.GAME_DOOM3, "tfphobos", "", "https://www.moddb.com/mods/phobos", 3),
    DOOM3_PHOBOS_PATCH(Q3EGameConstants.GAME_DOOM3, "tfphobos", "Compatibility patch", "https://www.moddb.com/games/doom-iii/addons/doom-3-phobos-dhewm3-compatibility-patch", 3),

    // Quake 4
    QUAKE4_BASE(Q3EGameConstants.GAME_QUAKE4, "q4base", "", "https://store.steampowered.com/app/2210/Quake_4/", 2),
    QUAKE4_HARDQORE(Q3EGameConstants.GAME_QUAKE4, "hardqore", "", "https://www.moddb.com/mods/quake-4-hardqore", 3),

    // Prey(2006)

    // Quake 1
    QUAKE1_BASE(Q3EGameConstants.GAME_QUAKE1, "id1", "", "https://store.steampowered.com/app/2310/Quake/", 2),

    // Quake 2
    QUAKE2_BASE(Q3EGameConstants.GAME_QUAKE2, "baseq2", "", "https://store.steampowered.com/app/2320/Quake_II/", 2),

    // Return to Castle Wolfenstein
    RTCW_BASE(Q3EGameConstants.GAME_RTCW, "main", "", "https://store.steampowered.com/app/9010/Return_to_Castle_Wolfenstein/", 2),

    // The dark mod
    TDM_BASE(Q3EGameConstants.GAME_TDM, "", "", "https://www.thedarkmod.com", 1),

    // Quake 3
    QUAKE3_BASE(Q3EGameConstants.GAME_QUAKE3, "baseq3", "", "https://store.steampowered.com/app/2200/Quake_III_Arena/", 2),
    QUAKE3_TEAMARENA(Q3EGameConstants.GAME_QUAKE3, "missionpack", "", "https://store.steampowered.com/app/2200/Quake_III_Arena/", 2),

    // Doom3 BFG
    D3BFG_BASE(Q3EGameConstants.GAME_DOOM3BFG, "base", "", "https://store.steampowered.com/app/208200/DOOM_3/", 2),
    D3BFG_RB(Q3EGameConstants.GAME_DOOM3BFG, "base", "", "https://www.moddb.com/mods/rbdoom-3-bfg", 3),

    // GZDOOM
    GZDOOM_DOOM1(Q3EGameConstants.GAME_GZDOOM, "doom", "", "https://store.steampowered.com/app/2280/DOOM__DOOM_II", 2),
    GZDOOM_DOOM2(Q3EGameConstants.GAME_GZDOOM, "doom2", "", "https://store.steampowered.com/app/2280/DOOM__DOOM_II", 2),

    // Wolfenstein: Enemy Territory
    ETW_BASE(Q3EGameConstants.GAME_ETW, "etmain", "ET: LEGACY", "https://www.etlegacy.com", 1),
    ETW_LEGACY(Q3EGameConstants.GAME_ETW, "etmain", "", "https://www.splashdamage.com/games/wolfenstein-enemy-territory/", 1),

    // RealRTCW
    REALRTCW_BASE(Q3EGameConstants.GAME_REALRTCW, "Main", "", "https://www.moddb.com/mods/realrtcw-realism-mod", 3),

    // FTEQW
    FTEQW_BASE(Q3EGameConstants.GAME_FTEQW, "", "", "https://www.fteqw.org", 1),
    FTEQW_Q1(Q3EGameConstants.GAME_FTEQW, "quake1", "", "https://store.steampowered.com/app/2310/Quake/", 2),
    FTEQW_Q2(Q3EGameConstants.GAME_FTEQW, "quake2", "", "https://store.steampowered.com/app/2320/Quake_II/", 2),
    FTEQW_Q3(Q3EGameConstants.GAME_FTEQW, "quake3", "", "https://store.steampowered.com/app/2200/Quake_III_Arena/", 2),
    FTEQW_H2(Q3EGameConstants.GAME_FTEQW, "hexen2", "", "https://store.steampowered.com/app/9060/HeXen_II/", 2),
    FTEQW_FREEHL(Q3EGameConstants.GAME_FTEQW, "halflife", "FreeHL", "https://github.com/eukara/freehl", 1),
    FTEQW_CS1_5(Q3EGameConstants.GAME_FTEQW, "cstrike_1_5", "FreeCS", "https://github.com/eukara/freecs", 1),


    JA_BASE(Q3EGameConstants.GAME_JA, "base", "", "https://store.steampowered.com/app/6020/STAR_WARS_Jedi_Knight__Jedi_Academy/", 2),
    JO_BASE(Q3EGameConstants.GAME_JO, "base", "", "https://store.steampowered.com/app/6030/STAR_WARS_Jedi_Knight_II__Jedi_Outcast/", 2),

    SAMTFE_BASE(Q3EGameConstants.GAME_SAMTFE, "", "", "https://store.steampowered.com/app/41050/", 2),

    SAMTSE_BASE(Q3EGameConstants.GAME_SAMTSE, "", "", "https://store.steampowered.com/app/41060/", 2),

    XASH_HLSDK(Q3EGameConstants.GAME_XASH3D, "", "", "https://github.com/FWGS/hlsdk-portable", 1),
    XASH_FWGS(Q3EGameConstants.GAME_XASH3D, "", "", "https://github.com/FWGS/xash3d-fwgs", 1),
    XASH_CS16(Q3EGameConstants.GAME_XASH3D, "", "", "https://github.com/Velaron/cs16-client", 1),

    SOURCE_ENGINE(Q3EGameConstants.GAME_SOURCE, "", "", "https://github.com/nillerusr/source-engine", 1),

    URT_BASE(Q3EGameConstants.GAME_URT, "", "", "https://www.urbanterror.info", 1),

    MOHAA_BASE(Q3EGameConstants.GAME_MOHAA, "", "", "https://github.com/openmoh/openmohaa", 1),
    ;

    public static final int SOURCE_HOMEPAGE = 1;
    public static final int SOURCE_STEAM    = 2;
    public static final int SOURCE_MODDB    = 3;
    public static final int SOURCE_OTHER    = 0;

    public final String type; // game type
    public final String game; // game id
    public final String name;
    public final String url;
    public final int    source;

    GameResourceUrl(String type, String game, String name, String url, int source)
    {
        this.type = type;
        this.game = game;
        this.name = name;
        this.url = url;
        this.source = source;
    }

    public static List<GameResourceUrl> GetResourceUrlList(String game, String mod)
    {
        GameResourceUrl[] values = GameResourceUrl.values();
        List<GameResourceUrl> list = new ArrayList<>();
        List<String> mods = new ArrayList<>();
        for(GameResourceUrl value : values)
        {
            if(!value.type.equals(game))
                continue;
            mods.add(value.game);
        }
        for(GameResourceUrl value : values)
        {
            if(!value.type.equalsIgnoreCase(game))
                continue;
            if(KStr.NotEmpty(mod) && mods.contains(mod) && !value.game.equalsIgnoreCase(mod))
                continue;
            list.add(value);
        }
        return list;
    }
}
