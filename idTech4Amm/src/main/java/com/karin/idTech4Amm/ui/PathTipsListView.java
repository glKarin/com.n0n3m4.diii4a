package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.widget.Divider;

import java.util.ArrayList;
import java.util.List;

/**
 * path tips view
 */
public class PathTipsListView extends ListView
{
    private PathTipsAdapter m_adapter;

    public PathTipsListView(Context context, List<PathTips> list)
    {
        super(context);
        SetupUI(list);
    }

    public void SetupUI(List<PathTips> list)
    {
        m_adapter = new PathTipsAdapter(getContext(), R.layout.pathtips_delegate);
        if(null != list)
        {
            List<PathTipsItem> items = new ArrayList<>();
            for (PathTips cg : list) {
                items.add(new PathTipsItem(cg));
            }
            m_adapter.SetData(items);
        }

        setAdapter(m_adapter);
        setClickable(false);
    }

    public void ExpandAll()
    {
        for (int i = 0; i < m_adapter.getCount(); i++) {
            m_adapter.getItem(i).hide = false;
        }
        m_adapter.notifyDataSetChanged();
    }

    public void CollapseAll()
    {
        for (int i = 0; i < m_adapter.getCount(); i++) {
            m_adapter.getItem(i).hide = true;
        }
        m_adapter.notifyDataSetChanged();
    }

    public static class ModPathTips
    {
        public final String mod;
        public final List<String> paths;

        public ModPathTips(String mod, List<String> paths)
        {
            this.mod = mod;
            this.paths = paths;
        }
    }

    public static class PathTips
    {
        public final String game;
        public final List<ModPathTips> mods;

        public PathTips(String game, List<ModPathTips> mods)
        {
            this.game = game;
            this.mods = mods;
        }
    }

    private static class PathTipsItem
    {
        public final String       name;
        public final CharSequence content;
        public boolean hide = false;

        public PathTipsItem(PathTips pt)
        {
            name = pt.game;
            final String endl = TextHelper.GetDialogMessageEndl();
            StringBuilder sb = new StringBuilder();
            for(int i = 0; i < pt.mods.size(); i++)
            {
                ModPathTips mod = pt.mods.get(i);
                sb.append(String.format("%2d", i + 1)).append(". ").append(mod.mod).append(": ").append(endl);
                for(String link : mod.paths)
                {
                    String pathText = TextHelper.GenLinkText("file://" + link, link);
                    sb.append(" * ").append(pathText).append(endl);
                }
            }
            content = TextHelper.GetDialogMessage(sb.toString());
        }
    }

    private static class PathTipsAdapter extends ArrayAdapter_base<PathTipsItem>
    {
        public PathTipsAdapter(Context context, int resource) {
            super(context, resource);
        }

        @Override
        public View GenerateView(int position, View view, ViewGroup parent, PathTipsItem data) {
            TextView contentView;
            Divider divider;

            divider = view.findViewById(R.id.pathtips_delegate_label);
            divider.SetText(data.name);
            divider.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    data.hide = !data.hide;
                    notifyDataSetChanged();
                }
            });

            contentView = view.findViewById(R.id.pathtips_delegate_content);
            contentView.setText(data.content);
            if(!TextHelper.USING_HTML)
                contentView.setAutoLinkMask(Linkify.ALL);
            contentView.setMovementMethod(LinkMovementMethod.getInstance());
            contentView.setVisibility(data.hide ? View.GONE : View.VISIBLE);

            return view;
        }
    }
}

