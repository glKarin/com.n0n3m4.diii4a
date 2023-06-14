package com.n0n3m4.q3e.onscreen;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.preference.PreferenceManager;

import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

public final class Q3EControls
{
    public static final int CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY = 30;
    public static final float CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE = 1.0f;
    public static final boolean CONST_DEFAULT_ON_SCREEN_BUTTON_FRIENDLY_EDGE = false;

    public static void SetupAllOpacity(Context context, int alpha, boolean save)
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        SharedPreferences.Editor mEdtr = preferences.edit();
        for (int i = 0; i < Q3EGlobals.UI_SIZE; i++)
        {
            String str = Q3EUtils.q3ei.defaults_table[i];
            int index = str.lastIndexOf(' ');
            str = str.substring(0, index) + ' ' + alpha;
            Q3EUtils.q3ei.defaults_table[i] = str;

            if(save)
            {
                String key = Q3EPreference.pref_controlprefix + i;
                if(!preferences.contains(key))
                    continue;
                str = preferences.getString(key, Q3EUtils.q3ei.defaults_table[i]);
                if(null == str)
                    str = Q3EUtils.q3ei.defaults_table[i];
                index = str.lastIndexOf(' ');
                str = str.substring(0, index) + ' ' + alpha;
                mEdtr.putString(key, str);
            }
        }
        if(save)
            mEdtr.commit();
    }

    public static void SetupAllSize(Activity context, float scale, boolean save)
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        SharedPreferences.Editor mEdtr = preferences.edit();
        int[] defSizes = GetDefaultSize(context);
        final boolean needScale = scale > 0.0f && scale != 1.0f;

        for (int i = 0; i < Q3EGlobals.UI_SIZE; i++)
        {
            int newSize = needScale ? Math.round((float)defSizes[i] * scale) : defSizes[i];

            String str = Q3EUtils.q3ei.defaults_table[i];
            String[] arr = str.split(" ");
            arr[2] = "" + newSize;
            str = Q3EUtils.Join(" ", arr);
            Q3EUtils.q3ei.defaults_table[i] = str;

            if(save)
            {
                String key = Q3EPreference.pref_controlprefix + i;
                if (!preferences.contains(key))
                    continue;
                str = preferences.getString(key, Q3EUtils.q3ei.defaults_table[i]);
                if (null == str)
                    str = Q3EUtils.q3ei.defaults_table[i];
                arr = str.split(" ");
                arr[2] = "" + newSize;
                str = Q3EUtils.Join(" ", arr);
                mEdtr.putString(key, str);
            }
        }
        if(save)
            mEdtr.commit();
    }

    public static String[] GetDefaultLayout(Activity context)
    {
        return GetDefaultLayout(context, CONST_DEFAULT_ON_SCREEN_BUTTON_FRIENDLY_EDGE, CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE, CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY);
    }

    public static String[] GetDefaultLayout(Activity context, boolean friendly, float scale, int opacity)
    {
        if(scale <= 0.0f)
            scale = 1.0f;

        int safeInsetTop = Q3EUtils.GetEdgeHeight(context, false);
        int safeInsetBottom = Q3EUtils.GetEndEdgeHeight(context, false);
        int[] fullSize = Q3EUtils.GetFullScreenSize(context);
        int[] size = Q3EUtils.GetNormalScreenSize(context);
        int navBarHeight = fullSize[1] - size[1] - safeInsetTop - safeInsetBottom;
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        boolean hideNav = preferences.getBoolean(Q3EPreference.HIDE_NAVIGATION_BAR, true);
        boolean coverEdges = preferences.getBoolean(Q3EPreference.COVER_EDGES, true);
        int w, h;
        int X = 0;
        if(friendly)
        {
            w = fullSize[0];
            h = fullSize[1];
            h -= navBarHeight;
            if(coverEdges)
                X = safeInsetTop;
            else
                h -= (safeInsetTop + safeInsetBottom);
        }
        else
        {
            w = fullSize[0];
            h = fullSize[1];
            if(!hideNav)
                h -= navBarHeight;
            if(!coverEdges)
                h -= (safeInsetTop + safeInsetBottom);
        }
        int width = Math.max(w, h);
        int height = Math.min(w, h);

        final boolean needScale = scale > 0.0f && scale != 1.0f;
        int r = Q3EUtils.dip2px(context,75);
        if(needScale)
            r = Math.round((float)r * scale);
        int rightoffset=r*3/4;
        int sliders_width=Q3EUtils.dip2px(context,125);
        if(needScale)
            sliders_width = Math.round((float)sliders_width * scale);
        final int alpha = opacity;

        String[] defaults_table=new String[Q3EGlobals.UI_SIZE];
        defaults_table[Q3EGlobals.UI_JOYSTICK] =(X + r*4/3)+" "+(height-r*4/3)+" "+r+" "+alpha;
        defaults_table[Q3EGlobals.UI_SHOOT]    =(width-r/2-rightoffset)+" "+(height-r/2-rightoffset)+" "+r*3/2+" "+alpha;
        defaults_table[Q3EGlobals.UI_JUMP]     =(width-r/2)+" "+(height-r-2*rightoffset)+" "+r+" "+alpha;
        defaults_table[Q3EGlobals.UI_CROUCH]   =(width-r/2)+" "+(height-r/2)+" "+r+" "+alpha;
        defaults_table[Q3EGlobals.UI_RELOADBAR]=(width-sliders_width/2-rightoffset/3)+" "+(sliders_width*3/8)+" "+sliders_width+" "+alpha;
        defaults_table[Q3EGlobals.UI_PDA]   =(width-r-2*rightoffset)+" "+(height-r/2)+" "+r+" "+alpha;
        defaults_table[Q3EGlobals.UI_FLASHLIGHT]     =(width-r/2-4*rightoffset)+" "+(height-r/2)+" "+r+" "+alpha;
        defaults_table[Q3EGlobals.UI_SAVE]     =(X + sliders_width/2)+" "+sliders_width/2+" "+sliders_width+" "+alpha;

        for (int i=Q3EGlobals.UI_SAVE+1;i<Q3EGlobals.UI_SIZE;i++)
            defaults_table[i]=(r/2+r*(i-Q3EGlobals.UI_SAVE-1))+" "+(height+r/2)+" "+r+" "+alpha;

        defaults_table[Q3EGlobals.UI_WEAPON_PANEL] =(width - sliders_width - r - rightoffset)+" "+(r)+" "+(r / 3)+" "+alpha;

        //k
        final int sr = r / 6 * 5;
        defaults_table[Q3EGlobals.UI_1] = String.format("%d %d %d %d", width - sr / 2 - sr * 2, (sliders_width * 5 / 8 + sr / 2), sr, alpha);
        defaults_table[Q3EGlobals.UI_2] = String.format("%d %d %d %d", width - sr / 2 - sr, (sliders_width * 5 / 8 + sr / 2), sr, alpha);
        defaults_table[Q3EGlobals.UI_3] = String.format("%d %d %d %d", width - sr / 2, (sliders_width * 5 / 8 + sr / 2), sr, alpha);
        defaults_table[Q3EGlobals.UI_KBD] = String.format("%d %d %d %d", X + sliders_width + sr / 2, sr / 2, sr, alpha);
        defaults_table[Q3EGlobals.UI_CONSOLE] = String.format("%d %d %d %d", X + sliders_width / 2 + sr / 2, sliders_width / 2 + sr / 2, sr, alpha);

        return defaults_table;
    }

    public static int[] GetDefaultSize(Activity context)
    {
        final String[] defs = GetDefaultLayout(context);
        int[] defSizes = new int[defs.length];
        for (int i = 0; i < defs.length; i++) {
            String[] arr = defs[i].split(" ");
            defSizes[i] = Integer.parseInt(arr[2]);
        }
        return defSizes;
    }

    public static Point[] GetDefaultPosition(Activity context, boolean friendly, float scale)
    {
        final String[] defs = GetDefaultLayout(context, friendly, scale, CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY);
        Point[] defPositions = new Point[defs.length];
        for (int i = 0; i < defs.length; i++) {
            String[] arr = defs[i].split(" ");
            defPositions[i] = new Point(Integer.parseInt(arr[0]), Integer.parseInt(arr[1]));
        }
        return defPositions;
    }

    public static Point[] GetDefaultPosition(Activity context, boolean friendly)
    {
        return GetDefaultPosition(context, friendly, CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE);
    }

    private Q3EControls() {}
}
