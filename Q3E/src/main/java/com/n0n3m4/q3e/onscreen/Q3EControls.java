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

    public static String[] GetDefaultLayout(Activity context, boolean landscape)
    {
        return GetDefaultLayout(context, CONST_DEFAULT_ON_SCREEN_BUTTON_FRIENDLY_EDGE, CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE, CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY, landscape);
    }

    private static String ButtonLayoutStr(Number x, Number y, Number r_or_w, Number a)
    {
        return x.intValue() + " " + y.intValue() + " " + r_or_w.intValue() + " " + a.intValue();
    }

    public static String[] GetDefaultLayout(Activity context, boolean friendly, float scale, int opacity, boolean landscape)
    {
        return Q3EButtonLayoutManager.GetDefaultLayout(context, friendly, scale, opacity, landscape, false);
    }

    public static String[] GetPortraitDefaultLayout(Activity context, boolean friendly, float scale, int opacity, boolean landscape)
    {
        return Q3EButtonLayoutManager.GetDefaultLayout(context, friendly, scale, opacity, landscape, true);
    }

    public static int[] GetDefaultSize(Activity context, boolean landscape)
    {
        final String[] defs = GetDefaultLayout(context, landscape);
        int[] defSizes = new int[defs.length];
        for (int i = 0; i < defs.length; i++)
        {
            String[] arr = defs[i].split(" ");
            defSizes[i] = Integer.parseInt(arr[2]);
        }
        return defSizes;
    }

    public static Point[] GetDefaultPosition(Activity context, boolean friendly, float scale, boolean landscape)
    {
        final String[] defs = GetDefaultLayout(context, friendly, scale, CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY, landscape);
        Point[] defPositions = new Point[defs.length];
        for (int i = 0; i < defs.length; i++)
        {
            String[] arr = defs[i].split(" ");
            defPositions[i] = new Point(Integer.parseInt(arr[0]), Integer.parseInt(arr[1]));
        }
        return defPositions;
    }

    public static Point[] GetDefaultPosition(Activity context, boolean friendly, boolean landscape)
    {
        return GetDefaultPosition(context, friendly, CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE, landscape);
    }

    private Q3EControls()
    {
    }
}
