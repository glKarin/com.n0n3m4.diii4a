package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.karin.idTech4Amm.R;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.Q3ELang;

import java.util.ArrayList;
import java.util.List;

public class ControlsThemeAdapter extends ArrayAdapter_base<ControlsTheme>
{
    private final List<ControlsTheme> m_list = new ArrayList<>();

    public ControlsThemeAdapter(Context context)
    {
        super(context, R.layout.controls_theme_list_delegate);

        Q3EInterface q3ei = Q3EUtils.q3ei;
        // joystick
        String str = Q3EUtils.q3ei.texture_table[Q3EGlobals.UI_JOYSTICK];
        if(null != str && !str.isEmpty())
        {
            String[] split = str.split(";");
            String name = split[0];
            ControlsTheme theme = new ControlsTheme();
            theme.name = Q3ELang.tr(getContext(), R.string.joystick_background);
            theme.path = name;
            m_list.add(theme);
            name = split[1];
            theme = new ControlsTheme();
            theme.name = Q3ELang.tr(getContext(), R.string.joystick_center);
            theme.path = name;
            m_list.add(theme);
        }

        for(int i = 0; i < Q3EGlobals.UI_SIZE; i++)
        {
            int type = q3ei.type_table[i];
            if(type == Q3EGlobals.TYPE_JOYSTICK || type == Q3EGlobals.TYPE_DISC)
                continue;
            String name = Q3EUtils.q3ei.texture_table[i];
            ControlsTheme theme = new ControlsTheme();
            theme.name = Q3EGlobals.CONTROLS_NAMES[i];
            theme.path = name;
            m_list.add(theme);
        }

        // weapon panel
        str = Q3EUtils.q3ei.texture_table[Q3EGlobals.UI_WEAPON_PANEL];
        if(null != str && !str.isEmpty())
        {
            String[] split = str.split(";");
            String name = split[0];
            ControlsTheme theme = new ControlsTheme();
            theme.name = Q3ELang.tr(getContext(), R.string.weapon_panel);
            theme.path = name;
            m_list.add(theme);
        }
        SetData(m_list);
    }

    @Override
    public View GenerateView(int position, View view, ViewGroup parent, ControlsTheme data)
    {
        ImageView image = view.findViewById(R.id.controls_theme_list_delegate_image);

        if(null != data.texture)
            image.setImageBitmap(data.texture);
        else
            image.setImageDrawable(new ColorDrawable(Color.BLACK));

        TextView textView = view.findViewById(R.id.controls_theme_list_delegate_name);
        textView.setText(data.name);
        textView = view.findViewById(R.id.controls_theme_list_delegate_path);
        textView.setText(data.path);
        textView = view.findViewById(R.id.controls_theme_list_delegate_size);
        textView.setText(data.GetTextureSize());

        return view;
    }

    public void Update(String themeName)
    {
        for (ControlsTheme theme : m_list)
        {
            theme.Release();
            theme.Load(getContext(), themeName);
        }
        notifyDataSetChanged();
    }
}

class ControlsTheme
{
    public String name;
    public String path;
    public Bitmap texture;

    public void Release()
    {
        if(null != texture)
        {
            texture.recycle();
            texture = null;
        }
    }

    public void Load(Context context, String type)
    {
        texture = Q3EUtils.LoadControlBitmap(context, path, type);
    }

    public String GetTextureSize()
    {
        return null != texture ? String.format("%d x %d", texture.getWidth(), texture.getHeight()) : "";
    }
}