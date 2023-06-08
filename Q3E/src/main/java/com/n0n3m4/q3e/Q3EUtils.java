/*
	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.os.Build;
import android.util.TypedValue;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.View;
import android.view.WindowInsets;
import android.view.inputmethod.InputMethodManager;

import com.n0n3m4.q3e.tv.Q3EOuya;

import java.io.InputStream;

public class Q3EUtils
{
    public static Q3EInterface q3ei = new Q3EInterface(); //k: new
    public static boolean isOuya = false;

    static
    {
        Q3EUtils.isOuya = Q3EOuya.IsValid();
    }

    public static final String pref_datapath = "q3e_datapath";
    public static final String pref_params = "q3e_params";
    public static final String pref_hideonscr = "q3e_hideonscr";
    public static final String pref_mapvol = "q3e_mapvol";
    public static final String pref_analog = "q3e_analog";
    public static final String pref_detectmouse = "q3e_detectmouse";
    public static final String pref_eventdev = "q3e_eventdev";
    public static final String pref_mousepos = "q3e_mousepos";
    public static final String pref_scrres = "q3e_scrres";
    public static final String pref_resx = "q3e_resx";
    public static final String pref_resy = "q3e_resy";
    public static final String pref_32bit = "q3e_32bit";
    public static final String pref_msaa = "q3e_msaa";
    public static final String pref_2fingerlmb = "q3e_2fingerlmb";
    public static final String pref_nolight = "q3e_nolight";
    public static final String pref_useetc1 = "q3e_useetc1";
    public static final String pref_usedxt = "q3e_usedxt";
    public static final String pref_useetc1cache = "q3e_useetc1cache";
    public static final String pref_controlprefix = "q3e_controls_";
    public static final String pref_harm_16bit = "q3e_harm_16bit"; //k
    public static final String pref_harm_r_harmclearvertexbuffer = "q3e_r_harmclearvertexbuffer"; //k
    public static final String pref_harm_fs_game = "q3e_harm_fs_game"; //k
    public static final String pref_harm_game_lib = "q3e_harm_game_lib"; //k
    public static final String pref_harm_r_specularExponent = "q3e_harm_r_specularExponent"; //k
    public static final String pref_harm_r_lightModel = "q3e_harm_r_lightModel"; //k
    public static final String pref_harm_mapBack = "q3e_harm_map_back"; //k
    public static final String pref_harm_game = "q3e_harm_game"; //k
    public static final String pref_harm_q4_fs_game = "q3e_harm_q4_fs_game"; //k
    public static final String pref_harm_q4_game_lib = "q3e_harm_q4_game_lib"; //k
    public static final String pref_harm_user_mod = "q3e_harm_user_mod"; //k
    public static final String pref_harm_view_motion_control_gyro = "q3e_harm_mouse_move_control_gyro"; //k
    public static final String pref_harm_view_motion_gyro_x_axis_sens = "q3e_harm_view_motion_gyro_x_axis_sens"; //k
    public static final String pref_harm_view_motion_gyro_y_axis_sens = "q3e_harm_view_motion_gyro_y_axis_sens"; //k
    public static final String pref_harm_auto_quick_load = "q3e_harm_auto_quick_load"; //k
    public static final String pref_harm_prey_fs_game = "q3e_harm_prey_fs_game"; //k
    public static final String pref_harm_prey_game_lib = "q3e_harm_prey_game_lib"; //k
    public static final String pref_harm_multithreading = "q3e_harm_multithreading"; //k
    public static final String pref_harm_s_driver = "q3e_harm_s_driver"; //k
    public static final String pref_harm_function_key_toolbar = "harm_function_key_toolbar"; //k
    public static final String pref_harm_function_key_toolbar_y = "harm_function_key_toolbar_y"; //k
    public static final String pref_harm_joystick_release_range = "harm_joystick_release_range"; //k
    public static final String pref_harm_joystick_unfixed = "harm_joystick_unfixed"; //k
    public static final String pref_harm_joystick_inner_dead_zone = "harm_joystick_inner_dead_zone"; //k

    public static boolean isAppInstalled(Activity ctx, String nm)
    {
        try
        {
            ctx.getPackageManager().getPackageInfo(nm, PackageManager.GET_ACTIVITIES);
            return true;
        } catch (Exception e)
        {
            return false;
        }
    }

    public static Bitmap ResourceToBitmap(Context cnt, String assetname)
    {
        try
        {
            InputStream is = cnt.getAssets().open(assetname);
            Bitmap b = BitmapFactory.decodeStream(is);
            is.close();
            return b;
        } catch (Exception ignored) {}
        return null;
    }

    public static int dip2px(Context ctx, int dip)
    {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dip, ctx.getResources().getDisplayMetrics());
    }

    public static int nextpowerof2(int x)
    {
        int candidate = 1;
        while (candidate < x)
            candidate *= 2;
        return candidate;
    }

    public static void togglevkbd(View vw)
    {
        InputMethodManager imm = (InputMethodManager) vw.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        if (q3ei.function_key_toolbar)
        {
            boolean changed = imm.hideSoftInputFromWindow(vw.getWindowToken(), 0);
            if (changed) // im from open to close
                ToggleToolbar(false);
            else // im is closed
            {
                //imm.showSoftInput(vw, InputMethodManager.SHOW_FORCED);
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
                ToggleToolbar(true);
            }
        } else
        {
            imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
        }
    }

    public static void ToggleToolbar(boolean on)
    {
        if (null != q3ei.callbackObj.vw)
            q3ei.callbackObj.vw.ToggleToolbar(on);
    }

    public static void CloseVKB()
    {
        if (null != q3ei.callbackObj.vw)
        {
            InputMethodManager imm = (InputMethodManager) q3ei.callbackObj.vw.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(q3ei.callbackObj.vw.getWindowToken(), 0);
        }
    }

    public static int[] GetNormalScreenSize(Activity activity)
    {
        Display display = activity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1)
        {
            display.getSize(size);
            return new int[]{size.x, size.y};
        } else
        {
            return new int[]{display.getWidth(), display.getHeight()};
        }
    }

    public static int[] GetFullScreenSize(Activity activity)
    {
        Display display = activity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
        {
            display.getRealSize(size);
            return new int[]{size.x, size.y};
        } else
        {
            return new int[]{display.getWidth(), display.getHeight()};
        }
    }

    public static int GetEdgeHeight(Activity activity, boolean landscape)
    {
        int safeInsetTop = 0;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
        {
            WindowInsets rootWindowInsets = activity.getWindow().getDecorView().getRootWindowInsets();
            if (null != rootWindowInsets)
            {
                DisplayCutout displayCutout = rootWindowInsets.getDisplayCutout();
                if (null != displayCutout)
                {
                    safeInsetTop = landscape ? displayCutout.getSafeInsetLeft() : displayCutout.getSafeInsetTop();
                }
            }
        }
        return safeInsetTop;
    }

    public static int GetEndEdgeHeight(Activity activity, boolean landscape)
    {
        int safeInsetBottom = 0;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
        {
            WindowInsets rootWindowInsets = activity.getWindow().getDecorView().getRootWindowInsets();
            if (null != rootWindowInsets)
            {
                DisplayCutout displayCutout = rootWindowInsets.getDisplayCutout();
                if (null != displayCutout)
                {
                    safeInsetBottom = landscape ? displayCutout.getSafeInsetRight() : displayCutout.getSafeInsetBottom();
                }
            }
        }
        return safeInsetBottom;
    }

    public static String GetGameLibDir(Context context)
    {
        try
        {
            ApplicationInfo ainfo = context.getApplicationContext().getPackageManager().getApplicationInfo
                    (
                            context.getPackageName(),
                            PackageManager.GET_SHARED_LIBRARY_FILES
                    );
            return ainfo.nativeLibraryDir; //k for arm64-v8a apk install
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return context.getCacheDir().getAbsolutePath().replace("cache", "lib");		//k old, can work with armv5 and armv7-a
        }
    }

    public static <T extends Comparable> T Clamp(T target, T min, T max)
    {
        return target.compareTo(min) < 0 ? min : (target.compareTo(max) > 0 ? max : target);
    }

    public static float Clampf(float target, float min, float max)
    {
        return Math.max(min, Math.min(target, max));
    }

    public static float Rad2Deg(double rad)
    {
        double deg = rad / Math.PI * 180.0;
        return FormatAngle((float) deg);
    }

    public static float FormatAngle(float deg)
    {
        while (deg > 360)
            deg -= 360;
        while (deg < 0)
            deg += 360.0;
        return deg;
    }
}
