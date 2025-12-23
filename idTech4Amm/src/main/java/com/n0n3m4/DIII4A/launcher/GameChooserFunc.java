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
import com.karin.idTech4Amm.ui.ArrayAdapter_base;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EInterface;

import java.util.ArrayList;
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
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.game);

        ListView list = new ListView(m_gameLauncher);
        GameAdapter adapter = new GameAdapter(list.getContext());
        list.setAdapter(adapter);

        builder.setView(list);
        AlertDialog dialog = builder.create();
        list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
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
        public int name;
        public String game;
        public Integer icon;
    }
}
