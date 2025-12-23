package com.karin.idTech4Amm.sys;

import com.karin.idTech4Amm.R;
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3EInterface;

import java.util.ArrayList;
import java.util.List;

public enum LauncherGame
{
    // KARIN_NEW_GAME_BOOKMARK

    DOOM3(Q3EGameConstants.GAME_ID_DOOM3, Q3EGameConstants.GAME_DOOM3, R.drawable.d3_icon, R.string.doom_iii, R.color.theme_doom3_main_color, R.string.doom_3),
    QUAKE4(Q3EGameConstants.GAME_ID_QUAKE4, Q3EGameConstants.GAME_QUAKE4, R.drawable.q4_icon, R.string.quake_iv_q4base, R.color.theme_quake4_main_color, R.string.quake_4),
    PREY(Q3EGameConstants.GAME_ID_PREY, Q3EGameConstants.GAME_PREY, R.drawable.prey_icon, R.string.prey_preybase, R.color.theme_prey_main_color, R.string.prey_2006),


    RTCW(Q3EGameConstants.GAME_ID_RTCW, Q3EGameConstants.GAME_RTCW, R.drawable.rtcw_icon, R.string.rtcw_base, R.color.theme_rtcw_main_color, R.string.rtcw),

    QUAKE3(Q3EGameConstants.GAME_ID_QUAKE3, Q3EGameConstants.GAME_QUAKE3, R.drawable.q3_icon, R.string.quake_3_base, R.color.theme_quake3_main_color, R.string.quake_3),

    QUAKE2(Q3EGameConstants.GAME_ID_QUAKE2, Q3EGameConstants.GAME_QUAKE2, R.drawable.q2_icon, R.string.quake_2_base, R.color.theme_quake2_main_color, R.string.quake_2),

    QUAKE1(Q3EGameConstants.GAME_ID_QUAKE1, Q3EGameConstants.GAME_QUAKE1, R.drawable.q1_icon, R.string.quake_1_base, R.color.theme_quake1_main_color, R.string.quake_1),

    DOOM3BFG(Q3EGameConstants.GAME_ID_DOOM3BFG, Q3EGameConstants.GAME_DOOM3BFG, R.drawable.d3bfg_icon, R.string.d3bfg_base, R.color.theme_d3bfg_main_color, R.string.doom_3_bfg),

    TDM(Q3EGameConstants.GAME_ID_TDM, Q3EGameConstants.GAME_TDM, R.drawable.tdm_icon, R.string.tdm_base, R.color.theme_tdm_main_color, R.string.tdm),

    GZDOOM(Q3EGameConstants.GAME_ID_GZDOOM, Q3EGameConstants.GAME_GZDOOM, R.drawable.gzdoom_icon, R.string.doom_base, R.color.theme_gzdoom_main_color, R.string.doom),

    ETW(Q3EGameConstants.GAME_ID_ETW, Q3EGameConstants.GAME_ETW, R.drawable.etw_icon, R.string.etw_base, R.color.theme_etw_main_color, R.string.etw),

    REALRTCW(Q3EGameConstants.GAME_ID_REALRTCW, Q3EGameConstants.GAME_REALRTCW, R.drawable.realrtcw_icon, R.string.realrtcw_base, R.color.theme_realrtcw_main_color, R.string.realrtcw),

    FTEQW(Q3EGameConstants.GAME_ID_FTEQW, Q3EGameConstants.GAME_FTEQW, R.drawable.fteqw_icon, R.string.fteqw_base, R.color.theme_fteqw_main_color, R.string.fteqw),

    JA(Q3EGameConstants.GAME_ID_JA, Q3EGameConstants.GAME_JA, R.drawable.ja_icon, R.string.openja_base, R.color.theme_ja_main_color, R.string.openja),
    JO(Q3EGameConstants.GAME_ID_JO, Q3EGameConstants.GAME_JO, R.drawable.jo_icon, R.string.openjo_base, R.color.theme_jo_main_color, R.string.openjo),

    SAMTFE(Q3EGameConstants.GAME_ID_SAMTFE, Q3EGameConstants.GAME_SAMTFE, R.drawable.samtfe_icon, R.string.samtfe_base, R.color.theme_samtfe_main_color, R.string.samtfe),
    SAMTSE(Q3EGameConstants.GAME_ID_SAMTSE, Q3EGameConstants.GAME_SAMTSE, R.drawable.samtse_icon, R.string.samtse_base, R.color.theme_samtse_main_color, R.string.samtse),

    XASH3D(Q3EGameConstants.GAME_ID_XASH3D, Q3EGameConstants.GAME_XASH3D, R.drawable.xash3d_icon, R.string.xash3d_base, R.color.theme_xash3d_main_color, R.string.xash3d),

    SOURCE(Q3EGameConstants.GAME_ID_SOURCE, Q3EGameConstants.GAME_SOURCE, R.drawable.source_icon, R.string.sourceengine_base, R.color.theme_source_main_color, R.string.sourceengine),

    URT(Q3EGameConstants.GAME_ID_URT, Q3EGameConstants.GAME_URT, R.drawable.urbanterror_icon, R.string.urbanterror_base, R.color.theme_urt_main_color, R.string.urbanterror),

    MOHAA(Q3EGameConstants.GAME_ID_MOHAA, Q3EGameConstants.GAME_MOHAA, R.drawable.mohaa_icon, R.string.openmohaa_base, R.color.theme_mohaa_main_color, R.string.openmohaa),
    ;

    public final int    GAME_ID;
    public final String TYPE;
    public final int    ICON_ID;
    public final int    NAME_ID;
    public final int    COLOR_ID;
    public final int    TYPE_ID;

    LauncherGame(int GAME_ID, String TYPE, int ICON_ID, int NAME_ID, int COLOR_ID, int TYPE_ID)
    {
        this.GAME_ID = GAME_ID;
        this.TYPE = TYPE;
        this.ICON_ID = ICON_ID;
        this.NAME_ID = NAME_ID;
        this.COLOR_ID = COLOR_ID;
        this.TYPE_ID = TYPE_ID;
    }

    public static LauncherGame Find(int index)
    {
        LauncherGame[] values = values();
        if(index >= 0 && index < values.length)
            return values[index];
        else
            return values[Q3EGameConstants.GAME_ID_DOOM3]; // DOOM3
    }

    public static LauncherGame Find(String type)
    {
        LauncherGame[] values = values();
        for(LauncherGame value : values)
        {
            if(value.TYPE.equalsIgnoreCase(type))
                return value;
        }
        return values[Q3EGameConstants.GAME_ID_DOOM3]; // DOOM3
    }

    public static String[] GameTypes(boolean all)
    {
        LauncherGame[] values = values();
        List<String> list = new ArrayList<>();
        for(LauncherGame value : values)
        {
            if(!all && Q3EInterface.IsDisabled(value.TYPE))
                continue;
            list.add(value.TYPE);
        }
        return list.toArray(new String[0]);
    }
}
