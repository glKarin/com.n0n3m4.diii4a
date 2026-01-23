package com.n0n3m4.q3e.onscreen;

import android.content.SharedPreferences;
import android.graphics.Rect;
import android.preference.PreferenceManager;
import android.view.View;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import javax.microedition.khronos.opengles.GL10;

public class UiLoader
{
    View ctx;
    public GL10 gl;
    int height;
    int width;
    //String filename;

    public String[] defaultsTable;

    public UiLoader(View cnt, GL10 gl10, int w, int h, String[] table)//, String fname)
    {
        ctx = cnt;
        gl = gl10;
        width = w;
        height = h;
        //filename=fname;
        defaultsTable = table;

        //Set defaults table
    }

    public Object LoadElement(int id, boolean editMode)
    {
        String tmp = defaultsTable[id];
        UiElement el = new UiElement(tmp, width, height);
        return LoadUiElement(id, el.cx, el.cy, el.size, el.alpha, editMode);
    }

    private Object LoadUiElement(int id, int cx, int cy, int size, int alpha, boolean editMode)
    {
        int key, key2, key3;
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(ctx.getContext());
        switch (Q3EUtils.q3ei.type_table[id])
        {
            case Q3EGlobals.TYPE_BUTTON:
                boolean disableMouseMotion = preferences.getBoolean(Q3EPreference.pref_harm_disable_mouse_button_motion, false);
                int bh = Button.HeightForWidth(size, Q3EUtils.q3ei.arg_table[id * 4 + 2]);
                key = Q3EKeyCodes.GetRealKeyCode(Q3EUtils.q3ei.arg_table[id * 4]);
                return new Button(ctx, gl, cx, cy, size, bh, Q3EUtils.q3ei.texture_table[id], key, Q3EUtils.q3ei.arg_table[id * 4 + 2], Q3EUtils.q3ei.arg_table[id * 4 + 1] == 1, !disableMouseMotion, (float) alpha / 100);
            case Q3EGlobals.TYPE_JOYSTICK: {
                int visibleMode = preferences.getInt(Q3EPreference.pref_harm_joystick_visible, Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_ALWAYS);
                boolean showDot = preferences.getBoolean(Q3EPreference.pref_harm_hide_joystick_center, false);
                float joystick_release_range = preferences.getFloat(Q3EPreference.pref_harm_joystick_release_range, 0.0f);
                float joystick_inner_dead_zone = preferences.getFloat(Q3EPreference.pref_harm_joystick_inner_dead_zone, 0.0f);
                boolean joystick_unfixed = preferences.getBoolean(Q3EPreference.pref_harm_joystick_unfixed, false);
                return new Joystick(ctx, gl, size, (float) alpha / 100, cx, cy, joystick_release_range, joystick_inner_dead_zone, !joystick_unfixed, !editMode, visibleMode, !showDot, Q3EUtils.q3ei.texture_table[id]);
            }
            case Q3EGlobals.TYPE_SLIDER:
                int sliderDelay = preferences.getInt(Q3EPreference.BUTTON_SWIPE_RELEASE_DELAY, Q3EGlobals.BUTTON_SWIPE_RELEASE_DELAY_AUTO);
                if(sliderDelay < 0)
                {
                    if(Q3EUtils.q3ei.isSamTFE || Q3EUtils.q3ei.isSamTSE)
                        sliderDelay = Q3EGlobals.SERIOUS_SAM_BUTTON_SWIPE_RELEASE_DELAY;
                }
                key = Q3EKeyCodes.GetRealKeyCode(Q3EUtils.q3ei.arg_table[id * 4]);
                key2 = Q3EKeyCodes.GetRealKeyCode(Q3EUtils.q3ei.arg_table[id * 4 + 1]);
                key3 = Q3EKeyCodes.GetRealKeyCode(Q3EUtils.q3ei.arg_table[id * 4 + 2]);
                int sh = Slider.HeightForWidth(size, Q3EUtils.q3ei.arg_table[id * 4 + 3]);
                return new Slider(ctx, gl, cx, cy, size, sh, Q3EUtils.q3ei.texture_table[id], key, key2, key3, Q3EUtils.q3ei.arg_table[id * 4 + 3], (float) alpha / 100, sliderDelay);
            case Q3EGlobals.TYPE_DISC:
            {
                int discKey = Q3EUtils.q3ei.arg_table[id * 4];
                if(discKey <= 0)
                    discKey = 1;
                else if(discKey > Q3EKeyCodes.ONSCRREN_DISC_KEYS_STRS.length)
                    discKey = 2;
                String keysStr = preferences.getString(Q3EUtils.q3ei.DiscPanelKeysPreference(false, discKey), null);
                if(KStr.IsEmpty(keysStr))
                    keysStr = preferences.getString(Q3EUtils.q3ei.DiscPanelKeysPreference(true, discKey), Q3EKeyCodes.ONSCRREN_DISC_KEYS_STRS[discKey - 1]);
                final int[] keycodes = Q3EKeyCodes.ONSCRREN_DISC_KEYS_KEYCODES[discKey - 1];
                final String[] labels = Q3EKeyCodes.ONSCRREN_DISC_KEYS_STRS[discKey - 1].split(",");
                char[] keys = null;
                char[] keymaps = null;
                if(KStr.NotBlank(keysStr))
                {
                    String[] arr = keysStr.split(",");
                    keys = new char[arr.length];
                    if(null != keycodes)
                        keymaps = new char[arr.length];
                    for(int i = 0; i < arr.length; i++)
                    {
                        String str = arr[i];
                        keys[i] = str.charAt(0);

                        if(null != keymaps)
                        {
                            for(int m = 0; m < labels.length; m++)
                            {
                                if(labels[m].equals(str))
                                {
                                    keymaps[i] = (char) keycodes[m];
                                    break;
                                }
                            }
                        }
                    }
                }

                int discName = Q3EUtils.q3ei.arg_table[id * 4 + 2];
                String name = null;
                if(discName != 0)
                {
                    StringBuilder buf = new StringBuilder();
                    long l = Integer.toUnsignedLong(discName);
                    for(int i = 0; i < 4; i++)
                    {
                        long c = (l >> (i * 8)) & 0xFF;
                        if(c == 0)
                            break;
                        buf.insert(0, (char) c);
                    }
                    name = buf.toString();
                }
                int discDelay = preferences.getInt(Q3EPreference.BUTTON_SWIPE_RELEASE_DELAY, Q3EGlobals.BUTTON_SWIPE_RELEASE_DELAY_AUTO);
                if(discDelay < 0)
                {
                    if(Q3EUtils.q3ei.isSamTFE || Q3EUtils.q3ei.isSamTSE)
                        discDelay = Q3EGlobals.SERIOUS_SAM_BUTTON_SWIPE_RELEASE_DELAY;
                }
                return new Disc(ctx, gl, cx, cy, size, (float) alpha / 100, keys, keymaps, Q3EUtils.q3ei.arg_table[id * 4 + 1], Q3EUtils.q3ei.texture_table[id], name, discDelay);
            }
        }
        return null;
    }

    public boolean CheckVisible(int id)
    {
        String tmp = defaultsTable[id];
        UiElement el = new UiElement(tmp, width, height);
        final Rect ScreenRect = new Rect(0, 0, width, height);
        Rect btnRect;
        switch (Q3EUtils.q3ei.type_table[id])
        {
            case Q3EGlobals.TYPE_BUTTON:
                int bh = el.size;
                if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_FULL)
                    bh =  el.size;
                else if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_RIGHT_BOTTOM)
                    bh =  el.size;
                else if (Q3EUtils.q3ei.arg_table[id * 4 + 2] == Q3EGlobals.ONSCREEN_BUTTON_TYPE_CENTER)
                    bh =  el.size / 2;
                btnRect = new Rect(-el.size / 2 + el.cx, -bh / 2 + el.cy, el.size / 2 + el.cx, bh / 2 + el.cy);
                return ScreenRect.intersect(btnRect);
            case Q3EGlobals.TYPE_JOYSTICK: {
                return true;
            }
            case Q3EGlobals.TYPE_SLIDER:
                int sh = el.size;
                if (Q3EUtils.q3ei.arg_table[id * 4 + 3] == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT || Q3EUtils.q3ei.arg_table[id * 4 + 3] == Q3EGlobals.ONSCRREN_SLIDER_STYLE_LEFT_RIGHT_SPLIT_CLICK)
                    sh = el.size / 2;
                btnRect = new Rect(-el.size / 2 + el.cx, -sh / 2 + el.cy, el.size / 2 + el.cx, sh / 2 + el.cy);
                return ScreenRect.intersect(btnRect);
            case Q3EGlobals.TYPE_DISC:
            {
                int r = el.size * 2;
                btnRect = new Rect(-r / 2 + el.cx, -r / 2 + el.cy, r / 2 + el.cx, r / 2 + el.cy);
                return ScreenRect.intersect(btnRect);
            }
        }
        return false;
    }
}
