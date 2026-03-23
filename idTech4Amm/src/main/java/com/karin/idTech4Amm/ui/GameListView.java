package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.sys.GameManager;
import com.karin.idTech4Amm.widget.Divider;

import java.util.ArrayList;
import java.util.List;

/**
 * game list view
 */
public class GameListView extends ListView
{
    private GameInfoAdapter m_adapter;
    private Callback callback;

    public static interface Callback
    {
        public void OnGameSelected(String game);
    }

    public GameListView(Context context, List<GameEngineInfo> list)
    {
        super(context);
        SetupUI(list);
    }

    public void SetCallback(Callback callback)
    {
        this.callback = callback;
    }

    public void SetupUI(List<GameEngineInfo> list)
    {
        m_adapter = new GameInfoAdapter(getContext(), R.layout.game_delegate);
        if(null != list)
        {
            List<GameEngineInfoItem> items = new ArrayList<>();
            for(GameEngineInfo cg : list)
            {
                items.add(new GameEngineInfoItem(cg));
            }
            m_adapter.SetData(items);
        }

        setAdapter(m_adapter);
        setClickable(false);
    }

    public void ExpandAll()
    {
        for(int i = 0; i < m_adapter.getCount(); i++)
        {
            m_adapter.getItem(i).hide = false;
        }
        m_adapter.notifyDataSetChanged();
    }

    public void CollapseAll()
    {
        for(int i = 0; i < m_adapter.getCount(); i++)
        {
            m_adapter.getItem(i).hide = true;
        }
        m_adapter.notifyDataSetChanged();
    }

    public static class GameEngineInfo
    {
        public final String       engineName;
        public final List<String> games;

        public GameEngineInfo(String engineName, List<String> games)
        {
            this.engineName = engineName;
            this.games = games;
        }
    }

    private static class GameInfoItem
    {
        public final String  game;
        public final int     name;
        public final Integer icon;
        public final Integer color;

        public GameInfoItem(String game, int name, Integer icon, Integer color)
        {
            this.game = game;
            this.name = name;
            this.icon = icon;
            this.color = color;
        }
    }

    private class GameEngineInfoItem
    {
        public final String             name;
        public final List<GameInfoItem> games;
        public       boolean            hide = false;

        public GameEngineInfoItem(GameEngineInfo ei)
        {
            name = ei.engineName;
            games = new ArrayList<>();
            for(String game : ei.games)
            {
                int colorId = GameManager.GetGameThemeColor(game);
                Resources resources = getContext().getResources();
                int color = resources.getColor(colorId);
                GameInfoItem item = new GameInfoItem(game, GameManager.GetGameNameRS(game), GameManager.GetGameIcon(game), color);
                games.add(item);
            }
        }
    }

    private class GameInfoAdapter extends ArrayAdapter_base<GameEngineInfoItem>
    {
        public GameInfoAdapter(Context context, int resource)
        {
            super(context, resource);
        }

        @Override
        public View GenerateView(int position, View view, ViewGroup parent, GameEngineInfoItem data)
        {
            LinearLayout contentView;
            Divider divider;

            divider = view.findViewById(R.id.game_delegate_label);
            divider.SetText(data.name);
            divider.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    data.hide = !data.hide;
                    notifyDataSetChanged();
                }
            });

            contentView = view.findViewById(R.id.game_delegate_content);
            contentView.removeAllViews();
            for(GameInfoItem game : data.games)
            {
                View v = LayoutInflater.from(contentView.getContext()).inflate(R.layout.game_list_delegate, contentView, false);

                ImageView image = v.findViewById(R.id.game_list_delegate_image);
                Resources resources = getContext().getResources();

                if(null != game.icon)
                    image.setImageDrawable(resources.getDrawable(game.icon));
                else
                    image.setImageDrawable(new ColorDrawable(Color.WHITE));

                TextView textView = v.findViewById(R.id.game_list_delegate_name);
                textView.setText(game.name);
                //textView.setTextColor(game.color);

                v.setOnClickListener(new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if(null != GameListView.this.callback)
                            GameListView.this.callback.OnGameSelected(game.game);
                    }
                });

                contentView.addView(v);
            }
            contentView.setVisibility(data.hide ? View.GONE : View.VISIBLE);

            return view;
        }
    }
}

