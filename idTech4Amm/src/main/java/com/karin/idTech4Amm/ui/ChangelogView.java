package com.karin.idTech4Amm.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Color;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ListView;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.misc.ChangeLog;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Theme;
import com.karin.idTech4Amm.widget.Divider;

import java.util.ArrayList;
import java.util.List;

/**
 * changelog view
 */
public class ChangelogView extends ListView
{
    private ChangelogAdapter m_adapter;

    public ChangelogView(Context context, ChangeLog[] list)
    {
        super(context);
        SetupUI(list);
    }

    public void SetupUI(ChangeLog[] list)
    {
        m_adapter = new ChangelogAdapter(getContext(), R.layout.changelog_delegate);
        if(null != list)
        {
            List<ChangelogItem> items = new ArrayList<>();
            for (ChangeLog changeLog : list) {
                items.add(new ChangelogItem(changeLog));
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

    private static class ChangelogItem
    {
        public final String label;
        public final CharSequence content;
        public boolean hide = false;

        public ChangelogItem(ChangeLog cl)
        {
            label = cl.date + " (R" + cl.release + ")";
            content = TextHelper.GetDialogMessage(cl.GenContentString(TextHelper.GetDialogMessageEndl()));
        }
    }

    private static class ChangelogAdapter extends ArrayAdapter_base<ChangelogItem>
    {
        public ChangelogAdapter(Context context, int resource) {
            super(context, resource);
        }

        @Override
        public View GenerateView(int position, View view, ViewGroup parent, ChangelogItem data) {
            TextView contentView;
            Divider divider;

            divider = view.findViewById(R.id.changelog_delegate_label);
            divider.SetText(data.label);
            divider.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    data.hide = !data.hide;
                    notifyDataSetChanged();
                }
            });

            contentView = view.findViewById(R.id.changelog_delegate_content);
            contentView.setText(data.content);
            if(!TextHelper.USING_HTML)
                contentView.setAutoLinkMask(Linkify.ALL);
            contentView.setMovementMethod(LinkMovementMethod.getInstance());
            contentView.setVisibility(data.hide ? View.GONE : View.VISIBLE);

            return view;
        }
    }
}

