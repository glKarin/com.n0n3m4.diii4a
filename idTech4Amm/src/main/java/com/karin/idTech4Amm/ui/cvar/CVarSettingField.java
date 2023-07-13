package com.karin.idTech4Amm.ui.cvar;

import android.content.Context;
import android.graphics.Color;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.karin.idTech4Amm.lib.KCVar;

public class CVarSettingField extends LinearLayout
{
    private CVarSettingInterface m_field;

    public CVarSettingField(Context context, String name, View view)
    {
        super(context);
        m_field = (CVarSettingInterface) view;
        Setup(name, view);
    }

    private void Setup(String name, View view)
    {
        Context context = getContext();
        LayoutParams params;

        setOrientation(VERTICAL);
        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        TextView label = new TextView(context);
        label.setTextSize(16);
        label.setText(name);
        label.setTextColor(Color.BLACK);
        addView(label, params);

        params = new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        params.setMargins(24, params.topMargin, params.rightMargin, params.bottomMargin);
        addView(view, params);
    }

    public CVarSettingInterface CVarSettingUI()
    {
        return m_field;
    }

    public static CVarSettingField Create(Context context, KCVar cvar)
    {
        View view = CVarSettingUI.GenerateSettingUI(context, cvar);
        if(null == view)
            return null;
        return new CVarSettingField(context, cvar.name, view);
    }
}
