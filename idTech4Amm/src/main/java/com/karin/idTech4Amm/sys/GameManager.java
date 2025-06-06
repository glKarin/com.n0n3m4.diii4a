package com.karin.idTech4Amm.sys;

import android.support.annotation.NonNull;

import com.karin.idTech4Amm.R;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

// game mod manager
public final class GameManager
{
    private final Map<String, List<GameProp>> GameProps = new LinkedHashMap<>();

    public final static String[] Games = {
            Q3EGlobals.GAME_DOOM3,
            Q3EGlobals.GAME_QUAKE4,
            Q3EGlobals.GAME_PREY,
            Q3EGlobals.GAME_RTCW,
            Q3EGlobals.GAME_QUAKE3,
            Q3EGlobals.GAME_QUAKE2,
            Q3EGlobals.GAME_QUAKE1,
            Q3EGlobals.GAME_DOOM3BFG,
            Q3EGlobals.GAME_TDM,
            Q3EGlobals.GAME_GZDOOM,
            Q3EGlobals.GAME_ETW,
            Q3EGlobals.GAME_REALRTCW,
            Q3EGlobals.GAME_FTEQW,
            Q3EGlobals.GAME_JA,
            Q3EGlobals.GAME_JO,
            Q3EGlobals.GAME_SAMTFE,
    };

    public static int[] GetGameIcons()
    {
        return new int[]{
                R.drawable.d3_icon,
                R.drawable.q4_icon,
                R.drawable.prey_icon,
                R.drawable.rtcw_icon,
                R.drawable.q3_icon,
                R.drawable.q2_icon,
                R.drawable.q1_icon,
                R.drawable.d3bfg_icon,
                R.drawable.tdm_icon,
                R.drawable.gzdoom_icon,
                R.drawable.etw_icon,
                R.drawable.realrtcw_icon,
                R.drawable.fteqw_icon,
                R.drawable.ja_icon,
                R.drawable.jo_icon,
                R.drawable.samtfe_icon,
        };
    }

    public static int[] GetGameNameRSs()
    {
        return new int[]{
                R.string.doom_iii,
                R.string.quake_iv_q4base,
                R.string.prey_preybase,
                R.string.rtcw_base,
                R.string.quake_3_base,
                R.string.quake_2_base,
                R.string.quake_1_base,
                R.string.d3bfg_base,
                R.string.tdm_base,
                R.string.doom_base,
                R.string.etw_base,
                R.string.realrtcw_base,
                R.string.fteqw_base,
                R.string.openja_base,
                R.string.openjo_base,
                R.string.samtfe_base,
        };
    }

    public static int[] GetGameThemeColors()
    {
        return new int[]{
                R.color.theme_doom3_main_color,
                R.color.theme_quake4_main_color,
                R.color.theme_prey_main_color,
                R.color.theme_rtcw_main_color,
                R.color.theme_quake3_main_color,
                R.color.theme_quake2_main_color,
                R.color.theme_quake1_main_color,
                R.color.theme_d3bfg_main_color,
                R.color.theme_tdm_main_color,
                R.color.theme_gzdoom_main_color,
                R.color.theme_etw_main_color,
                R.color.theme_realrtcw_main_color,
                R.color.theme_fteqw_main_color,
                R.color.theme_ja_main_color,
                R.color.theme_jo_main_color,
                R.color.theme_samtfe_main_color,
        };
    }

    public static int[] GetGameNameTSs()
    {
        return new int[]{
                R.string.doom_3,
                R.string.quake_4,
                R.string.prey_2006,
                R.string.rtcw,
                R.string.quake_3,
                R.string.quake_2,
                R.string.quake_1,
                R.string.doom_3_bfg,
                R.string.tdm,
                R.string.doom,
                R.string.etw,
                R.string.realrtcw,
                R.string.fteqw,
                R.string.openja,
                R.string.openjo,
                R.string.samtfe,
        };
    }

    public GameManager()
    {
        InitGameProps();
    }

    public static class GameProp
    {
        public final int     index;
        public final String  game;
        public final String  fs_game;
        public final String  fs_game_base;
        public final boolean is_user;
        public final String  lib;
        public final String  file;

        public GameProp(int index, String game, String fs_game, String fs_game_base, boolean is_user, String lib, String file)
        {
            this.index = index;
            this.game = game;
            this.fs_game = fs_game;
            this.fs_game_base = fs_game_base;
            this.is_user = is_user;
            this.lib = lib;
            if(null == file)
                this.file = fs_game;
            else
                this.file = file;
        }

        public boolean IsGame(String game)
        {
            if (null == game)
                game = "";
            if(game.equals(this.game))
                return true;
            if(index == 0 && game.isEmpty())
                return true;
            return false;
        }

        public boolean IsValid()
        {
            return index >= 0 && !game.isEmpty();
        }
    }

    private void InitGameProps()
    {
        List<GameProp> props;
        GameProp prop;

        for(String game : Games)
            GameProps.put(game, new ArrayList<>());
        Game[] values = Game.values();

        for (Game value : values)
        {
            props = GameProps.get(value.type);
            prop = new GameProp(props.size(), value.game, value.fs_game, value.fs_game_base, false, value.lib, value.file);
            props.add(prop);
        }
    }

    public GameProp ChangeGameMod(String game, boolean userMod)
    {
        if (null == game)
            game = "";

        List<GameProp> list = GameProps.get(Q3EUtils.q3ei.game);

        GameProp res = null;
        for (GameProp prop : list)
        {
            if(prop.IsGame(game))
            {
                res = prop;
                break;
            }
        }
        if(null == res)
            res = new GameProp(0, "", game, "", userMod, "", null);
        return res;
    }

    public String GetGameOfMod(String game)
    {
        for (String key : GameProps.keySet())
        {
            List<GameProp> props = GameProps.get(key);
            for (GameProp prop : props)
            {
                if(prop.game.equals(game))
                    return key;
            }
        }
        return null;
    }

    public String GetGameFileOfMod(String game)
    {
        if(null == game)
            game = "";
        for (String key : GameProps.keySet())
        {
            List<GameProp> props = GameProps.get(key);
            for (GameProp prop : props)
            {
                if(prop.fs_game.equals(game))
                    return prop.file;
            }
        }
        return null;
    }

    public List<GameProp> GetGame(String game)
    {
        return GameProps.get(game);
    }

    public static int GetGameIcon()
    {
        return GetGameIcons()[Q3EUtils.q3ei.game_id];
    }

    public static int GetGameIcon(String name)
    {
        return GetGameIcons()[Q3EInterface.GetGameID(name)];
    }

    public static int GetGameName()
    {
        return GetGameNameRSs()[Q3EUtils.q3ei.game_id];
    }

    public static int GetGameNameRS(String name)
    {
        return GetGameNameRSs()[Q3EInterface.GetGameID(name)];
    }

    public static int GetGameThemeColor()
    {
        return GetGameThemeColors()[Q3EUtils.q3ei.game_id];
    }

    public static int GetGameNameTs(String name)
    {
        return GetGameNameTSs()[Q3EInterface.GetGameID(name)];
    }

    public String[] GetGameLibs(String name, boolean makePlatform)
    {
        List<GameProp> gameProps = GameProps.get(name);
        List<String> list = new ArrayList<>();
        for (GameProp gameProp : gameProps)
        {
            if(!list.contains(gameProp.lib))
                list.add(gameProp.lib);
        }
        if(!makePlatform)
            return list.toArray(new String[0]);
        else
        {
            String[] res = new String[list.size()];
            for (int i = 0; i < list.size(); i++)
            {
                res[i] = "lib" + list.get(i) + ".so";
            }
            return res;
        }
    }
}
