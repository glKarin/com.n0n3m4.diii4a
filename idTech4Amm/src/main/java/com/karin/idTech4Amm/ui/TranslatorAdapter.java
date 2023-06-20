package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.Constants;

import java.util.ArrayList;
import java.util.List;

public class TranslatorAdapter extends ArrayAdapter_base<Translator>
{
    public TranslatorAdapter(Context context)
    {
        super(context, R.layout.translators_list_delegate);

        List<Translator> m_list = new ArrayList<>();

        Translator tr;

        tr = new Translator();
        tr.lang = "English";
        tr.author = "n0n3m4";
        m_list.add(tr);

        tr = new Translator();
        tr.lang = "中文";
        tr.author = "Karin Zhao";
        tr.url = Constants.CONST_EMAIL;
        m_list.add(tr);

        tr = new Translator();
        tr.lang = "Русский";
        tr.author = "ALord7";
        tr.group = "4pda";
        tr.url = "https://4pda.ru/forum/index.php?showuser=5043340";
        m_list.add(tr);

        SetData(m_list);
    }

    @Override
    public View GenerateView(int position, View view, ViewGroup parent, Translator data)
    {
        TextView textView = view.findViewById(R.id.translators_list_delegate_lang);
        textView.setText(data.lang);
        textView = view.findViewById(R.id.translators_list_delegate_author);
        textView.setText(data.Name());
        textView.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                data.Open(getContext());
            }
        });

        return view;
    }
}

class Translator
{
    public String lang;
    public String author;
    public String group;
    public String url;

    public String Name()
    {
        String str = author;
        if(null != group)
            str += "@" + group;
        return str;
    }

    public void Open(Context context)
    {
        if(null == url || url.isEmpty())
            return;
        try
        {
            if(url.startsWith("http"))
                ContextUtility.OpenUrlExternally(context, url);
            else
            {
                ContextUtility.OpenUrlExternally(context, "mailto:" + url);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}