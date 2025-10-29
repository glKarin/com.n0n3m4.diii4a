package com.karin.idTech4Amm.sys;

import com.n0n3m4.q3e.Q3EUtils;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

// game mod manager
public final class GameManager
{
    private final Map<String, List<GameProp>> GameProps = new LinkedHashMap<>();

    public static String[] Games(boolean all)
    {
        return LauncherGame.GameTypes(all);
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

        for(String game : Games(true))
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
        return LauncherGame.Find(Q3EUtils.q3ei.game_id).ICON_ID;
    }

    public static int GetGameIcon(String game)
    {
        return LauncherGame.Find(game).ICON_ID;
    }

    public static int GetGameName()
    {
        return LauncherGame.Find(Q3EUtils.q3ei.game_id).NAME_ID;
    }

    public static int GetGameNameRS(String game)
    {
        return LauncherGame.Find(game).NAME_ID;
    }

    public static int GetGameThemeColor()
    {
        return LauncherGame.Find(Q3EUtils.q3ei.game_id).COLOR_ID;
    }

    public static int GetGameThemeColor(String game)
    {
        return LauncherGame.Find(game).COLOR_ID;
    }

    public static int GetGameNameTs(String game)
    {
        return LauncherGame.Find(game).TYPE_ID;
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
