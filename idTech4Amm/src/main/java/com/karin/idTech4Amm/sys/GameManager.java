package com.karin.idTech4Amm.sys;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

// game mod manager
public final class GameManager
{
    private final Map<String, List<GameProp>> GameProps = new LinkedHashMap<>();

    public GameManager()
    {
        InitGameProps();
    }

    public static class GameProp
    {
        public final int index;
        public final String game;
        public final String fs_game;
        public final String fs_game_base;
        //public boolean harm_fs_gameLibPath = true;
        public final boolean is_user;

        public GameProp(int index, String game, String fs_game, String fs_game_base, boolean is_user)
        {
            this.index = index;
            this.game = game;
            this.fs_game = fs_game;
            this.fs_game_base = fs_game_base;
            this.is_user = is_user;
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

        GameProps.put(Q3EGlobals.GAME_DOOM3, new ArrayList<>());
        GameProps.put(Q3EGlobals.GAME_QUAKE4, new ArrayList<>());
        GameProps.put(Q3EGlobals.GAME_PREY, new ArrayList<>());
        GameProps.put(Q3EGlobals.GAME_QUAKE2, new ArrayList<>());
        Game[] values = Game.values();

        for (Game value : values)
        {
            props = GameProps.get(value.type);
            prop = new GameProp(props.size(), value.game, value.fs_game, value.fs_game_base, false);
            props.add(prop);
        }
    }

    public GameProp ChangeGameMod(String game, boolean userMod)
    {
        if (null == game)
            game = "";

        List<GameProp> list;
        if (Q3EUtils.q3ei.isQ4)
        {
            list = GameProps.get(Q3EGlobals.GAME_QUAKE4);
        }
        else if (Q3EUtils.q3ei.isPrey)
        {
            list = GameProps.get(Q3EGlobals.GAME_PREY);
        }
        else if (Q3EUtils.q3ei.isQ2)
        {
            list = GameProps.get(Q3EGlobals.GAME_QUAKE2);
        }
        else
        {
            list = GameProps.get(Q3EGlobals.GAME_DOOM3);
        }

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
            res = new GameProp(0, "", game, "", userMod);
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
}
