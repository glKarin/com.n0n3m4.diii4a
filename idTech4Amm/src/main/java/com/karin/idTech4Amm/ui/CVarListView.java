package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.KCVar;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.widget.Divider;

import java.util.ArrayList;
import java.util.List;

/**
 * cvar view
 */
public class CVarListView extends ListView
{
    private CVarAdapter m_adapter;

    public CVarListView(Context context, List<KCVar.Group> list)
    {
        super(context);
        SetupUI(list);
    }

    public void SetupUI(List<KCVar.Group> list)
    {
        m_adapter = new CVarAdapter(getContext(), R.layout.cvar_delegate);
        if(null != list)
        {
            List<CVarItem> items = new ArrayList<>();
            for (KCVar.Group cg : list) {
                items.add(new CVarItem(cg));
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

    private static class CVarItem
    {
        public final String       category;
        public final CharSequence content;
        public boolean hide = false;

        public CVarItem(KCVar.Group cg)
        {
            category = cg.name;
            content = TextHelper.GetDialogMessage(cg.GetCvarText(TextHelper.GetDialogMessageEndl()));
        }
    }

    private static class CVarAdapter extends ArrayAdapter_base<CVarItem>
    {
        public CVarAdapter(Context context, int resource) {
            super(context, resource);
        }

        @Override
        public View GenerateView(int position, View view, ViewGroup parent, CVarItem data) {
            TextView contentView;
            Divider divider;

            divider = view.findViewById(R.id.cvar_delegate_label);
            divider.SetText(data.category);
            divider.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    data.hide = !data.hide;
                    notifyDataSetChanged();
                }
            });

            contentView = view.findViewById(R.id.cvar_delegate_content);
            contentView.setText(data.content);
            if(!TextHelper.USING_HTML)
                contentView.setAutoLinkMask(Linkify.ALL);
            contentView.setMovementMethod(LinkMovementMethod.getInstance());
            contentView.setVisibility(data.hide ? View.GONE : View.VISIBLE);

            return view;
        }
    }
}

