package com.n0n3m4.q3e.onscreen;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.View;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

import javax.microedition.khronos.opengles.GL10;

public class UiLoader
{
    View ctx;
    public GL10 gl;
    int height;
    int width;
    //String filename;

    public static String[] defaults_table;

    public UiLoader(View cnt, GL10 gl10, int w, int h)//, String fname)
    {
        ctx = cnt;
        gl = gl10;
        width = w;
        height = h;
        //filename=fname;
        defaults_table = Q3EUtils.q3ei.defaults_table;

        //Set defaults table
    }

    public Object LoadElement(int id, boolean editMode)
    {
        SharedPreferences shp = PreferenceManager.getDefaultSharedPreferences(ctx.getContext());
        String tmp = shp.getString(Q3EPreference.pref_controlprefix + id, null);
        if (tmp == null) tmp = defaults_table[id];
        UiElement el = new UiElement(tmp);
        return LoadUiElement(id, el.cx, el.cy, el.size, el.alpha, editMode);
    }

    private Object LoadUiElement(int id, int cx, int cy, int size, int alpha, boolean editMode)
    {
        int bh = size;
        if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == 0)
            bh = size;
        if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == 1)
            bh = size;
        if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == 2)
            bh = size / 2;

        int sh = size;
        if (Q3EUtils.q3ei.arg_table[id * 4 + 3] == 0)
            sh = size / 2;
        if (Q3EUtils.q3ei.arg_table[id * 4 + 3] == 1)
            sh = size;


        switch (Q3EUtils.q3ei.type_table[id])
        {
            case Q3EGlobals.TYPE_BUTTON:
                return new Button(ctx, gl, cx, cy, size, bh, Q3EUtils.q3ei.texture_table[id], Q3EUtils.q3ei.arg_table[id * 4], Q3EUtils.q3ei.arg_table[id * 4 + 2], Q3EUtils.q3ei.arg_table[id * 4 + 1] == 1, (float) alpha / 100);
            case Q3EGlobals.TYPE_JOYSTICK: {
                return new Joystick(ctx, gl, size, (float) alpha / 100, cx, cy, Q3EUtils.q3ei.joystick_release_range, Q3EUtils.q3ei.joystick_inner_dead_zone, Q3EUtils.q3ei.joystick_unfixed, editMode);
            }
            case Q3EGlobals.TYPE_SLIDER:
                return new Slider(ctx, gl, cx, cy, size, sh, Q3EUtils.q3ei.texture_table[id], Q3EUtils.q3ei.arg_table[id * 4], Q3EUtils.q3ei.arg_table[id * 4 + 1], Q3EUtils.q3ei.arg_table[id * 4 + 2], Q3EUtils.q3ei.arg_table[id * 4 + 3], (float) alpha / 100);
            case Q3EGlobals.TYPE_DISC:
            {
                String keysStr = PreferenceManager.getDefaultSharedPreferences(ctx.getContext()).getString(Q3EPreference.WEAPON_PANEL_KEYS, "1,2,3,4,5,6,7,8,9,q,0");
                char[] keys = null;
                if (null != keysStr && !keysStr.isEmpty())
                {
                    String[] arr = keysStr.split(",");
                    keys = new char[arr.length];
                    for (int i = 0; i < arr.length; i++)
                    {
                        keys[i] = arr[i].charAt(0);
                    }
                }
                return new Disc(ctx, gl, cx, cy, size, (float) alpha / 100, keys);
            }
        }
        return null;
    }
}
