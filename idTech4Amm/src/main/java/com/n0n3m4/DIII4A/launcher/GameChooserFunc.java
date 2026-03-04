package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.sys.GameManager;
import com.karin.idTech4Amm.sys.LauncherGame;
import com.karin.idTech4Amm.ui.ArrayAdapter_base;
import com.karin.idTech4Amm.ui.GameListView;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EGame;
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3ELang;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class GameChooserFunc extends GameLauncherFunc
{
    public GameChooserFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        run();
    }

    public void run()
    {
        ShowGroupListView();
        //ShowPlainListView();
    }

    private GameListView.GameEngineInfo CreateEngineInfo(Object engineName, String... engineIds)
    {
        List<String> games = new ArrayList<>();

        String[] types = LauncherGame.GameTypes(false);
        for(String engineId : engineIds)
        {
            for(String game : types)
            {
                Q3EGame info = Q3EGame.Find(game);
                if(!info.ENGINE.equals(engineId))
                    continue;

                games.add(info.TYPE);
            }
        }

        if(games.isEmpty())
            return null;

        String name;
        if(null == engineName)
            name = String.join("/", engineIds);
        else
        {
            if(engineName instanceof Integer)
                name = Q3ELang.tr(m_gameLauncher, (Integer)engineName);
            else
                name = engineName.toString();
        }
        return new GameListView.GameEngineInfo(name, games);
    }

    private List<GameListView.GameEngineInfo> CreateEngineInfos()
    {
        List<GameListView.GameEngineInfo> list = new ArrayList<>();
        class EngineItem {
            public final Object name;
            public final String[] list;

            public EngineItem(Object name, String...list)
            {
                this.name = name;
                this.list = list;
            }
        }

        List<EngineItem> configs = Arrays.asList(
                new EngineItem(R.string.idtech_4_engine, Q3EGameConstants.ENGINE_IDTECH_4),
                new EngineItem(R.string.idtech_3_engine, Q3EGameConstants.ENGINE_IDTECH_3),
                new EngineItem(R.string.idtech_2_engine, Q3EGameConstants.ENGINE_IDTECH_2),
                new EngineItem(R.string.quake_engine, Q3EGameConstants.ENGINE_ID_QUAKE),
                new EngineItem(R.string.idtech_1_engine, Q3EGameConstants.ENGINE_IDTECH_1),
                new EngineItem(R.string.based_on_idsoftware_engine, Q3EGameConstants.ENGINE_ID_BASE),
                new EngineItem(R.string.gold_source_source_engine, Q3EGameConstants.ENGINE_GOLD_SOURCE, Q3EGameConstants.ENGINE_SOURCE),
                new EngineItem(R.string.other_engine, Q3EGameConstants.ENGINE_OTHER)
        );

        for(EngineItem config : configs)
        {
            GameListView.GameEngineInfo engineInfo = CreateEngineInfo(config.name, config.list);
            if(null != engineInfo)
                list.add(engineInfo);
        }

        return list;
    }

    private void ShowGroupListView()
    {
        List<GameListView.GameEngineInfo> gameEngineInfos = CreateEngineInfos();
        GameListView gameListView = new GameListView(m_gameLauncher, gameEngineInfos);

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.game);
        builder.setView(gameListView);
        AlertDialog dialog = builder.create();
        gameListView.SetCallback(new GameListView.Callback() {
            @Override
            public void OnGameSelected(String game) {
                Callback(game);
                dialog.dismiss();
            }
        });
        dialog.create();
        dialog.show();
    }

    private void ShowPlainListView()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.game);

        ListView list = new ListView(m_gameLauncher);
        GameAdapter adapter = new GameAdapter(list.getContext());
        list.setAdapter(adapter);

        builder.setView(list);
        AlertDialog dialog = builder.create();
        list.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id)
            {
                Callback(adapter.getItem(position).game);
                dialog.dismiss();
            }
        });
        dialog.show();
    }

    private static class GameAdapter extends ArrayAdapter_base<GameItem>
    {
        private final List<GameItem> m_list = new ArrayList<>();

        public GameAdapter(Context context)
        {
            super(context, R.layout.game_list_delegate);

            for(String game : GameManager.Games(false))
            {
                GameItem item = new GameItem();
                item.game = game;
                item.icon = GameManager.GetGameIcon(game);
                item.name = GameManager.GetGameNameRS(game);
                m_list.add(item);
            }

            SetData(m_list);
        }

        @Override
        public View GenerateView(int position, View view, ViewGroup parent, GameItem data)
        {
            ImageView image = view.findViewById(R.id.game_list_delegate_image);
            Resources resources = getContext().getResources();

            if(null != data.icon)
                image.setImageDrawable(resources.getDrawable(data.icon));
            else
                image.setImageDrawable(new ColorDrawable(Color.WHITE));

            TextView textView = view.findViewById(R.id.game_list_delegate_name);
            textView.setText(data.name);

            return view;
        }
    }

    private static class GameItem
    {
        public int     name;
        public String  game;
        public Integer icon;
    }
}
