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
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.TypedValue;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.View;
import android.view.WindowInsets;
import android.view.inputmethod.InputMethodManager;

import com.n0n3m4.q3e.device.Q3EOuya;

import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.LinkedHashMap;

public class Q3EUtils
{
    public static Q3EInterface q3ei = new Q3EInterface(); //k: new
    public static boolean isOuya = false;

    static
    {
        Q3EUtils.isOuya = Q3EOuya.IsValid();
    }

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
        String type = PreferenceManager.getDefaultSharedPreferences(cnt).getString(Q3EPreference.CONTROLS_THEME, "");
        if(null == type)
            type = "";
        return LoadControlBitmap(cnt, assetname, type);
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

    // 1. try find in /sdcard/Android/data/<package>/files/assets
    // 2. try find in /<apk>/assets
    public static InputStream OpenResource(Context cnt, String assetname)
    {
        InputStream is;
        if((is = OpenResource_external(cnt, assetname)) == null)
            is = OpenResource_assets(cnt, assetname);
        return is;
    }

    public static InputStream OpenResource_external(Context cnt, String assetname)
    {
        InputStream is = null;
        try
        {
            final String filePath = GetAppStoragePath(cnt, "/assets/" + assetname);
            File file = new File(filePath);
            if(file.exists() && file.isFile() && file.canRead())
            {
                is = new FileInputStream(file);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return is;
    }

    public static InputStream OpenResource_assets(Context cnt, String assetname)
    {
        InputStream is = null;
        try
        {
            is = cnt.getAssets().open(assetname);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return is;
    }

    public static String GetAppStoragePath(Context context, String filename)
    {
        String path;
        File externalFilesDir = context.getExternalFilesDir(null);
        if(null != externalFilesDir)
            path = externalFilesDir.getAbsolutePath();
        else
            path = Environment.getExternalStorageDirectory() + "/Android/data/" + Q3EGlobals.CONST_PACKAGE_NAME + "/files";
        if(null != filename && !filename.isEmpty())
            path += filename;
        return path;
    }

    public static void Close(Closeable closeable)
    {
        try
        {
            if(null != closeable)
                closeable.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public static LinkedHashMap<String, String> GetControlsThemes(Context context)
    {
        LinkedHashMap<String, String> list = new LinkedHashMap<>();
        list.put("/android_asset", "Default");
        list.put("", "External");
        String filePath = GetAppStoragePath(context, "/assets/controls_theme");
        File dir = new File(filePath);
        if(dir.exists() && dir.isDirectory())
        {
            File[] files = dir.listFiles();
            for (File file : files)
            {
                if(file.isDirectory())
                    list.put("controls_theme/" + file.getName(), file.getName());
            }
        }
        return list;
    }

    public static Bitmap LoadControlBitmap(Context context, String path, String type)
    {
        InputStream is = null;
        Bitmap texture = null;
        switch (type)
        {
            case "/android_asset":
                is = Q3EUtils.OpenResource_assets(context, path);
                break;
            case "":
                is = Q3EUtils.OpenResource(context, path);
                break;
            default:
                if(type.startsWith("/"))
                {
                    type = type.substring(1);
                    is = Q3EUtils.OpenResource_assets(context, type + "/" + path);
                }
                else
                {
                    if((is = Q3EUtils.OpenResource(context, type + "/" + path)) == null)
                        is = Q3EUtils.OpenResource_assets(context, path);
                }
                break;
        }

        try
        {
            texture = BitmapFactory.decodeStream(is);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        finally
        {
            Q3EUtils.Close(is);
        }
        return texture;
    }

    public static int SupportMouse()
    {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ? Q3EGlobals.MOUSE_EVENT : Q3EGlobals.MOUSE_DEVICE;
    }
}
