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
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.Process;
import android.preference.PreferenceManager;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.Surface;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import com.n0n3m4.q3e.device.Q3EMouseDevice;
import com.n0n3m4.q3e.device.Q3EOuya;
import com.n0n3m4.q3e.karin.KFDManager;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KControlsTheme;
import com.n0n3m4.q3e.karin.KStr;

import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Date;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

public class Q3EUtils
{
    private static final String TAG = "Q3EUtils";
    public static Q3EInterface q3ei = new Q3EInterface(); //k: new
    public static boolean isOuya = false;
    public static int UI_FULLSCREEN_HIDE_NAV_OPTIONS = 0;
    public static int UI_FULLSCREEN_OPTIONS = 0;

    static
    {
        Q3EUtils.isOuya = Q3EOuya.IsValid();
        UI_FULLSCREEN_HIDE_NAV_OPTIONS = GetFullScreenFlags(true);
        UI_FULLSCREEN_OPTIONS = GetFullScreenFlags(false);
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
            boolean changed;
            if(q3ei.builtin_virtual_keyboard)
            {
                Q3E.activity.GetKeyboard().ToggleBuiltInVKB();
                changed = Q3E.activity.GetKeyboard().IsBuiltInVKBVisible();
                ToggleToolbar(changed);
            }
            else
            {
                changed = imm.hideSoftInputFromWindow(vw.getWindowToken(), 0);
                if (changed) // im from open to close
                    ToggleToolbar(false);
                else // im is closed
                {
                    //imm.showSoftInput(vw, InputMethodManager.SHOW_FORCED);
                    imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
                    ToggleToolbar(true);
                }
            }
        }
        else
        {
            if(q3ei.builtin_virtual_keyboard)
                Q3E.activity.GetKeyboard().ToggleBuiltInVKB();
            else
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
        }
    }

    public static void ToggleToolbar(boolean on)
    {
        q3ei.callbackObj.ToggleToolbar(on);
    }

    public static void OpenVKB(View vw)
    {
        if (null != vw)
        {
            InputMethodManager imm = (InputMethodManager) vw.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            if(q3ei.builtin_virtual_keyboard)
                Q3E.activity.GetKeyboard().OpenBuiltInVKB();
            else
            {
                //imm.showSoftInput(vw, InputMethodManager.SHOW_FORCED);
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
            }
        }
        if (Q3EUtils.q3ei.function_key_toolbar)
            Q3EUtils.ToggleToolbar(true);
    }

    public static void CloseVKB(View vw)
    {
        if (null != vw)
        {
            if(q3ei.builtin_virtual_keyboard)
            {
                Q3E.activity.GetKeyboard().CloseBuiltInVKB();
            }
            else
            {
                InputMethodManager imm = (InputMethodManager) vw.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(vw.getWindowToken(), 0);
            }
        }
        if (Q3EUtils.q3ei.function_key_toolbar)
            Q3EUtils.ToggleToolbar(false);
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

    public static boolean ActiveIsInvert(Activity activity)
    {
        int rotation;
        if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R)
        {
            rotation = activity.getDisplay().getRotation();
        }
        else
        {
            rotation = ((WindowManager) (activity.getSystemService(Context.WINDOW_SERVICE))).getDefaultDisplay().getRotation();
        }
        return (rotation == Surface.ROTATION_270 || rotation == Surface.ROTATION_180);
    }

    public static boolean ActiveIsPortrait(Activity activity)
    {
        int rotation;
        if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R)
        {
            rotation = activity.getDisplay().getRotation();
        }
        else
        {
            rotation = ((WindowManager) (activity.getSystemService(Context.WINDOW_SERVICE))).getDefaultDisplay().getRotation();
        }
        return (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180);
    }

    public static boolean ActiveIsLandscape(Activity activity)
    {
        int rotation;
        if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R)
        {
            rotation = activity.getDisplay().getRotation();
        }
        else
        {
            rotation = ((WindowManager) (activity.getSystemService(Context.WINDOW_SERVICE))).getDefaultDisplay().getRotation();
        }
        return (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270);
    }

    public static int GetStatusBarHeight(Activity activity)
    {
        int result = 0;

        Resources resources = activity.getResources();
        int resourceId = resources.getIdentifier("status_bar_height","dimen", "android");

        if (resourceId > 0)
            result = resources.getDimensionPixelSize(resourceId);

        return result;
    }

    public static int GetNavigationBarHeight(Activity activity, boolean landscape)
    {
        int[] fullSize = GetFullScreenSize(activity);
        int[] size = GetNormalScreenSize(activity);
        return fullSize[1] - size[1] - GetEdgeHeight(activity, landscape) - GetEndEdgeHeight(activity, landscape);
    }

    public static String GetGameLibDir(Context context)
    {
        try
        {
            ApplicationInfo ainfo = context.getApplicationContext().getPackageManager().getApplicationInfo
                    (
                            context.getApplicationContext().getPackageName(),
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

    public static InputStream OpenResource(Context cnt, String assetname)
    {
        InputStream is;
        is = KFDManager.Instance(cnt).OpenRead(assetname);
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

    public static String GetDataPath(String filename)
    {
        String path = "";
        if(null != q3ei.datadir)
            path += q3ei.datadir;
        if(KStr.NotEmpty(filename))
            path += filename;
        return path;
    }

    public static String GetAppStoragePath(Context context)
    {
        String path = Q3EUtils.GetAppStoragePath(context, null);
        File dir = new File(path);
        return dir.getParent();
    }

    public static String GetAppStoragePath(Context context, String filename)
    {
        String path;
        File externalFilesDir = context.getExternalFilesDir(null);
        if(null != externalFilesDir)
            path = externalFilesDir.getAbsolutePath();
        else
            path = Environment.getExternalStorageDirectory() + "/" + Q3EGlobals.CONST_PACKAGE_NAME + "/files";
        if(null != filename && !filename.isEmpty())
            path += filename;
        return path;
    }

    public static String GetDefaultGameDirectory(Context context)
    {
        String path = null;
/*        if(Build.VERSION.SDK_INT > Build.VERSION_CODES.P)
        {
            File externalFilesDir = context.getExternalFilesDir(null);
            if(null != externalFilesDir)
                path = externalFilesDir.getAbsolutePath();
            else
                path = Environment.getExternalStorageDirectory() + "/" + context.getApplicationContext().getPackageName() + "/files";
        }
        if(KStr.IsEmpty(path))*/
            path = Environment.getExternalStorageDirectory().getAbsolutePath();
        return path + "/diii4a";
    }

    public static String GetAppInternalPath(Context context, String filename)
    {
        String path;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            path = context.getDataDir().getAbsolutePath();
        else
            path = context.getCacheDir().getAbsolutePath() + "/..";
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

    public static List<KControlsTheme> GetControlsThemes(Context context)
    {
        KFDManager fs = KFDManager.Instance(context);
        List<KControlsTheme> list = new ArrayList<>();
        list.add(new KControlsTheme("/android_asset", Q3ELang.tr(context, R.string._default)));
        list.add(new KControlsTheme("", Q3ELang.tr(context, R.string.external)));
        List<String> controls_theme = fs.ListDir("controls_theme");
        for (String file : controls_theme)
        {
            String path = "controls_theme/" + file;

            KControlsTheme item = new KControlsTheme(path, file);

            List<String> searches = new ArrayList<>();
            searches.add("");
            String lang = Q3ELang.AppLang(context);
            if(KStr.NotEmpty(lang))
            {
                int i = lang.indexOf("-");
                if(i > 0)
                {
                    String l = lang.substring(0, i);
                    if(!searches.contains(l))
                        searches.add("." + l);
                }
                 if(!searches.contains(lang))
                    searches.add("." + lang);
            }

            for(String search : searches)
            {
                String desc = fs.ReadAsText(path + "/description" + search + ".txt");
                if(null != desc)
                {
                    item.Parse(desc);
                }
            }

            list.add(item);
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
        // return Q3EGlobals.MOUSE_EVENT;
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.O || !Q3EMouseDevice.DeviceIsRoot() ? Q3EGlobals.MOUSE_EVENT : Q3EGlobals.MOUSE_DEVICE;
    }

    public static String Join(String d, String...strs)
    {
        if(null == strs)
            return null;
        if(strs.length == 0)
            return "";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < strs.length; i++) {
            sb.append(strs[i]);
            if(i < strs.length - 1)
                sb.append(d);
        }
        return sb.toString();
    }

    public static float parseFloat_s(String str, float...def)
    {
        float defVal = null != def && def.length > 0 ? def[0] : 0.0f;
        if(null == str)
            return defVal;
        str = str.trim();
        if(str.isEmpty())
            return defVal;
        try
        {
            return Float.parseFloat(str);
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return defVal;
        }
    }

    public static int parseInt_s(String str, int...def)
    {
        int defVal = null != def && def.length > 0 ? def[0] : 0;
        if(null == str)
            return defVal;
        str = str.trim();
        if(str.isEmpty())
            return defVal;
        try
        {
            return Integer.parseInt(str);
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return defVal;
        }
    }

    public static long Copy(OutputStream out, InputStream in, int...bufferSizeArg) throws RuntimeException
    {
        if(null == out)
            return -1;
        if(null == in)
            return -1;

        int bufferSize = bufferSizeArg.length > 0 ? bufferSizeArg[0] : 0;
        if (bufferSize <= 0)
            bufferSize = 8192;

        byte[] buffer = new byte[bufferSize];

        long size = 0L;

        int readSize;
        try
        {
            while((readSize = in.read(buffer)) != -1)
            {
                out.write(buffer, 0, readSize);
                size += readSize;
                out.flush();
            }
        }
        catch (IOException e)
        {
            throw new RuntimeException(e);
        }

        return size;
    }

    public static String Read(InputStream in) throws RuntimeException
    {
        if(null == in)
            return "";

        ByteArrayOutputStream os = new ByteArrayOutputStream();
        try
        {
            Copy(os, in);
            byte[] bytes = os.toByteArray();
            return new String(bytes, StandardCharsets.UTF_8);
        }
        catch (Exception e)
        {
            throw new RuntimeException(e);
        }
        finally
        {
            Q3EUtils.Close(os);
        }
    }

    public static long cp(String src, String dst)
    {
        FileInputStream is = null;
        FileOutputStream os = null;
        File srcFile = new File(src);
        if(!srcFile.isFile())
            return -2;
        if(!srcFile.canRead())
            return -3;
        File dstFile = new File(dst);
        File dstDir = dstFile.getParentFile();
        if(null != dstDir && !dstDir.isDirectory())
        {
            if(!dstDir.mkdirs())
                return -4;
        }
        try
        {
            is = new FileInputStream(srcFile);
            os = new FileOutputStream(dst);
            return Q3EUtils.Copy(os, is);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return -1;
        }
        finally
        {
            Q3EUtils.Close(is);
            Q3EUtils.Close(os);
        }
    }

    /*
    @SuppressLint("InlinedApi")
    private final int m_uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN;
    @SuppressLint("InlinedApi")
    private final int m_uiOptions_def = View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
     */
    public static int GetFullScreenFlags(boolean hideNav)
    {
        int m_uiOptions = 0;

        if(hideNav)
        {
            m_uiOptions |= View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
            {
                m_uiOptions |= View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
        {
            m_uiOptions |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
            m_uiOptions |= View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
            m_uiOptions |= View.SYSTEM_UI_FLAG_FULLSCREEN;
        }
        return m_uiOptions;
    }

    public static void CopyToClipboard(Context context, String text)
    {
        ClipboardManager clipboard = (ClipboardManager)context.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("idTech4A++", text);
        clipboard.setPrimaryClip(clip);
    }

    public static String GetClipboardText(Context context)
    {
        ClipboardManager clipboard = (ClipboardManager)context.getSystemService(Context.CLIPBOARD_SERVICE);

        String pasteData = null;
        if (clipboard.hasPrimaryClip())
        {
            ClipData primaryClip = clipboard.getPrimaryClip();
            if(null != primaryClip && primaryClip.getItemCount() > 0)
            {
                ClipData.Item item = primaryClip.getItemAt(0);
                pasteData = item.getText().toString();
            }
        }
        return pasteData;
    }

    public static int[] CalcSizeByScaleScreenArea(int width, int height, BigDecimal scale)
    {
        double p = Math.sqrt(scale.doubleValue());
        BigDecimal bp = BigDecimal.valueOf(p);
        BigDecimal bw = new BigDecimal(width);
        BigDecimal bh = new BigDecimal(height);
        BigDecimal ww = bw.multiply(bp);
        int w = ww.intValue();
        int h = ww.multiply(bh).divide(bw, 2, RoundingMode.HALF_UP).intValue();
        return new int[]{w, h};
    }

    public static int[] CalcSizeByScaleWidthHeight(int width, int height, BigDecimal scale)
    {
        int w = new BigDecimal(width).multiply(scale).intValue();
        int h = new BigDecimal(height).multiply(scale).intValue();
        return new int[]{w, h};
    }

    public static boolean file_put_contents(String path, String content)
    {
        if(null == path)
            return false;
        return file_put_contents(new File(path), content);
    }

    public static boolean file_put_contents(File file, String content)
    {
        if(null == file)
            return false;

        FileWriter writer = null;
        try
        {
            writer = new FileWriter(file);
            writer.append(content);
            writer.flush();
            return true;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Close(writer);
        }
    }

    public static String file_get_contents(String path)
    {
        if(null == path)
            return null;
        return file_get_contents(new File(path));
    }

    public static String file_get_contents(File file)
    {
        if(null == file || !file.isFile() || !file.canRead())
            return null;

        FileReader reader = null;
        try
        {
            reader = new FileReader(file);
            int BUF_SIZE = 1024;
            char[] chars = new char[BUF_SIZE];
            int len;
            StringBuilder sb = new StringBuilder();
            while ((len = reader.read(chars)) > 0)
                sb.append(chars, 0, len);
            return sb.toString();
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return null;
        }
        finally
        {
            Close(reader);
        }
    }

    public static boolean rm(String path)
    {
        if(KStr.IsEmpty(path))
            return false;
        return rm(new File(path));
    }

    public static boolean rm(File file)
    {
        if(null == file || !file.isFile())
            return false;

        try
        {
            boolean ok = file.delete();
            KLog.D("rm: " + file.getAbsolutePath() + " -> " + (ok ? "success" : "fail"));
            return ok;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public static boolean rmdir(String path)
    {
        if(KStr.IsEmpty(path))
            return false;
        return rmdir(new File(path));
    }

    public static boolean rmdir(File file)
    {
        if(null == file || !file.isDirectory())
            return false;

        String[] list = file.list();
        if(null != list && list.length > 0)
            return false;

        try
        {
            boolean ok = file.delete();
            KLog.D("rmdir: " + file.getAbsolutePath() + " -> " + (ok ? "success" : "fail"));
            return ok;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public static boolean rmdir_r(String path)
    {
        if(KStr.IsEmpty(path))
            return false;
        return rmdir_r(new File(path));
    }

    public static boolean rmdir_r(File file)
    {
        if(null == file || !file.isDirectory())
            return false;

        File[] files = file.listFiles();
        if(null == files || files.length == 0)
            return true;

        try
        {
            for(File f : files)
            {
                if(!rm_r(f))
                    return false;
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public static boolean rm_r(String path)
    {
        if(KStr.IsEmpty(path))
            return false;
        return rm_r(new File(path));
    }

    public static boolean rm_r(File file)
    {
        if(null == file)
            return false;

        if(file.isDirectory())
        {
            File[] files = file.listFiles();
            if(null != files && files.length > 0)
            {
                for(File f : files)
                {
                    if(!rm_r(f))
                        return false;
                }
            }
            return rmdir(file);
        }
        else
            return rm(file);
    }

    public static String FileDir(String path)
    {
        if(path.endsWith("/") || path.endsWith("\\"))
            return path;
        return new File(path).getParentFile().getAbsolutePath();
    }

    public static boolean mkdir(String path, boolean p)
    {
        File file = new File(path);
        if(file.exists())
        {
            return file.isDirectory();
        }
        try
        {
            if(p)
                return file.mkdirs();
            else
                return file.mkdir();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }

    private static boolean _dumpPID = false;
    public static void DumpPID(Context context)
    {
        if(_dumpPID || !BuildConfig.DEBUG )
            return;
        try
        {
            String text = "" + Process.myPid();
            final String[] Paths;
            Paths = new String[]{
                    PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.pref_datapath, ""),
                    android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N ? context.getDataDir().getAbsolutePath() : context.getCacheDir().getAbsolutePath()
            };
            for (String dir : Paths)
            {
                if(null == dir || dir.isEmpty())
                    continue;
                String path = dir + "/.idtech4amm.pid";
                file_put_contents(path, text);
                Log.i(TAG, "DumpPID " + text + " to " + path);
                _dumpPID = true;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
    public static float GetRefreshRate(Context context)
    {
        WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = windowManager.getDefaultDisplay();
        return display.getRefreshRate();
    }

    public static void RunLauncher(Activity activity)
    {
        Intent intent = activity.getPackageManager().getLaunchIntentForPackage(activity.getApplicationContext().getPackageName());
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        activity.startActivity(intent);
    }

    public static int[] GetSurfaceViewSize(Context context, int screenWidth, int screenHeight)
    {
        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        int scheme = mPrefs.getInt(Q3EPreference.pref_scrres_scheme, Q3EGlobals.SCREEN_FULL);
        int scale = mPrefs.getInt(Q3EPreference.pref_scrres_scale, 100);
        BigDecimal scalef = new BigDecimal(scale).divide(new BigDecimal("100"), 2, RoundingMode.UP);
        if(scalef.compareTo(BigDecimal.ZERO) <= 0)
            scalef = BigDecimal.ONE;
        int width, height;
        switch (scheme)
        {
            case Q3EGlobals.SCREEN_SCALE_BY_LENGTH: {
                int[] size = CalcSizeByScaleWidthHeight(screenWidth, screenHeight, scalef);
                width = size[0];
                height = size[1];
            }
                break;
            case Q3EGlobals.SCREEN_SCALE_BY_AREA: {
                int[] size = CalcSizeByScaleScreenArea(screenWidth, screenHeight, scalef);
                width = size[0];
                height = size[1];
            }
                break;
            case Q3EGlobals.SCREEN_CUSTOM: {
                width = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_resx, "0"));
                height = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_resy, "0"));
                if (width <= 0 && height <= 0)
                {
                    width = screenWidth;
                    height = screenHeight;
                }
                if (width <= 0)
                {
                    width = (int)((float)height * (float)screenWidth / (float)screenHeight);
                }
                else if (height <= 0)
                {
                    height = (int)((float)width * (float)screenHeight / (float)screenWidth);
                }
            }
                break;
            case Q3EGlobals.SCREEN_FIXED_RATIO: {
                int ratiox = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_ratiox, "0"));
                int ratioy = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_ratioy, "0"));
                boolean useCustom = mPrefs.getBoolean(Q3EPreference.pref_ratio_use_custom_resolution, false);
                width = screenWidth;
                height = screenHeight;
                if(useCustom)
                {
                    width = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_resx, "0"));
                    height = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_resy, "0"));
                    if (width <= 0 && height > 0)
                    {
                        width = (int)((float)height * (float)screenWidth / (float)screenHeight);
                    }
                    else if (height <= 0 && width > 0)
                    {
                        height = (int)((float)width * (float)screenHeight / (float)screenWidth);
                    }
                }
                int[] sizes = CalcSizeByRatio(width, height, ratiox, ratioy);
                width = sizes[2];
                height = sizes[3];
            }
            break;
            case Q3EGlobals.SCREEN_FULL:
            default:
                width = screenWidth;
                height = screenHeight;
                break;
        }
        return new int[]{width, height};
    }

    public static String GetAppInternalSearchPath(Context context, String path)
    {
        if(null == path)
            path = "";
        return Q3EUtils.GetAppStoragePath(context, "/diii4a" + path);
    }

    public static String date_format(String format, Date...date)
    {
        Date d = null != date && date.length > 0 && null != date[0] ? date[0] : new Date();
        return new SimpleDateFormat(format).format(d);
    }

    public static long Write(String filePath, byte[] in) throws RuntimeException
    {
        FileOutputStream fileoutputstream = null;

        try
        {
            fileoutputstream = new FileOutputStream(filePath);
            fileoutputstream.write(in);
            fileoutputstream.flush();
            return in.length;
        }
        catch(IOException e)
        {
            throw new RuntimeException(e);
        }
        finally
        {
            Q3EUtils.Close(fileoutputstream);
        }
    }

    public static long Write(String filePath, InputStream in, int...bufferSizeArg) throws RuntimeException
    {
        FileOutputStream fileoutputstream = null;

        try
        {
            fileoutputstream = new FileOutputStream(filePath);
            return Q3EUtils.Copy(fileoutputstream, in, bufferSizeArg);
        }
        catch(FileNotFoundException e)
        {
            throw new RuntimeException(e);
        }
        finally
        {
            Q3EUtils.Close(fileoutputstream);
        }
    }

    public static boolean ExtractCopyFile(Context context, String assetFilePath, String systemFilePath, boolean overwrite)
    {
        InputStream bis = null;

        try
        {
            File srcFile = new File(assetFilePath);
            String systemFolderPath;
            String toFilePath;
            if(systemFilePath.endsWith("/"))
            {
                systemFolderPath = systemFilePath.substring(0, systemFilePath.length() - 1);
                toFilePath = systemFilePath + srcFile.getName();
            }
            else
            {
                File f = new File(systemFilePath);
                systemFolderPath = f.getParent();
                toFilePath = systemFilePath;
            }

            Q3EUtils.mkdir(systemFolderPath, true);

            bis = context.getAssets().open(assetFilePath);

            File file = new File(toFilePath);
            if(!overwrite && file.exists())
                return true;

            Q3EUtils.mkdir(file.getParent(), true);

            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying " + assetFilePath + " to " + toFilePath);
            Q3EUtils.Write(toFilePath, bis, 4096);
            bis.close();
            bis = null;
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(bis);
        }
    }

    public static boolean ExtractCopyDirFiles(Context context, String assetFolderPath, String systemFolderPath, boolean overwrite, String...assetPaths)
    {
        InputStream bis = null;

        try
        {
            Q3EUtils.mkdir(systemFolderPath, true);

            List<String> fileList;
            if(null == assetPaths)
            {
                String[] list = context.getAssets().list(assetFolderPath);
                if(null == list)
                    fileList = new ArrayList<>();
                else
                    fileList = Arrays.asList(list);
            }
            else
                fileList = Arrays.asList(assetPaths);

            for (String assetPath : fileList)
            {
                String sourcePath = assetFolderPath + "/" + assetPath;
                String entryName = systemFolderPath + "/" + assetPath;
                ExtractCopyFile(context, sourcePath, entryName, overwrite);
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(bis);
        }
    }

    public static boolean ExtractZip(Context context, String assetPath, String systemFolderPath, boolean overwrite)
    {
        InputStream bis = null;
        ZipInputStream zipinputstream = null;

        try
        {
            bis = context.getAssets().open(assetPath);
            zipinputstream = new ZipInputStream(bis);

            ZipEntry zipentry;
            Q3EUtils.mkdir(systemFolderPath, true);
            while ((zipentry = zipinputstream.getNextEntry()) != null)
            {
                String tmpname = zipentry.getName();

                String toFilePath = systemFolderPath + "/" + tmpname;
                toFilePath = toFilePath.replace('/', File.separatorChar);
                toFilePath = toFilePath.replace('\\', File.separatorChar);
                File file = new File(toFilePath);

                if (zipentry.isDirectory())
                {
                    if(!file.exists())
                        Q3EUtils.mkdir(toFilePath, true);
                    continue;
                }

                if(!overwrite && file.exists())
                    continue;

                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Extracting " + tmpname + " to " + systemFolderPath);
                Q3EUtils.Write(toFilePath, zipinputstream, 4096);
                zipinputstream.closeEntry();
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(zipinputstream);
            Q3EUtils.Close(bis);
        }
    }

    public static boolean ExtractCopyDir(Context context, String assetFolderPath, String systemFolderPath, boolean overwrite)
    {
        InputStream bis = null;

        try
        {
            List<String> fileList = LsAssets(context, assetFolderPath);
            if(null == fileList)
                return false;

            Q3EUtils.mkdir(systemFolderPath, true);

            for (String assetPath : fileList)
            {
                String sourcePath = assetFolderPath + "/" + assetPath;
                String entryName = systemFolderPath + "/" + assetPath;
                ExtractCopyFile(context, sourcePath, entryName, overwrite);
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(bis);
        }
    }

    public static boolean ExtractZipToZip(Context context, String assetPath, String systemFolderPath, boolean overwrite, String...files)
    {
        InputStream bis = null;
        ZipInputStream zipinputstream = null;
        ZipOutputStream outStream = null;
        ByteArrayOutputStream bos = null;

        File file = new File(systemFolderPath);

        if(!overwrite && file.exists())
            return false;

        try
        {
            Q3EUtils.mkdir(file.getParent(), true);

            bis = context.getAssets().open(assetPath);
            zipinputstream = new ZipInputStream(bis);
            bos = new ByteArrayOutputStream();
            outStream = new ZipOutputStream(bos);

            ZipEntry zipentry;
            while ((zipentry = zipinputstream.getNextEntry()) != null)
            {
                String tmpname = zipentry.getName();
                if(zipentry.isDirectory())
                {
                    // System.out.println("Skip dir" + tmpname);
                    zipinputstream.closeEntry();
                    continue;
                }

                boolean found = false;
                if(null != files)
                {
                    for(String rule : files)
                    {
                        if(rule.endsWith("/"))
                        {
                            if(tmpname.startsWith(rule))
                            {
                                found = true;
                                break;
                            }
                        }
                        else
                        {
                            if(tmpname.equals(rule))
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                }
                if(!found)
                {
                    //System.out.println("Skip " + tmpname);
                    zipinputstream.closeEntry();
                    continue;
                }

                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Extracting " + tmpname + " to " + systemFolderPath);

                outStream.putNextEntry(new ZipEntry(tmpname));
                Q3EUtils.Copy(outStream, zipinputstream, 4096);
                outStream.closeEntry();
                zipinputstream.closeEntry();
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(zipinputstream);
            Q3EUtils.Close(bis);
            Q3EUtils.Close(outStream);
            Q3EUtils.Close(bos);
        }

        Q3EUtils.Write(systemFolderPath, bos.toByteArray());
        return true;
    }

    public static boolean InMainThread(Context context)
    {
        return context.getMainLooper().getThread() == Thread.currentThread();
    }

    public static void RunOnUiThread(Context context, Runnable runnable)
    {
        if(context instanceof Activity)
        {
            ((Activity)context).runOnUiThread(runnable);
        }
        else
        {
            if(InMainThread(context))
                runnable.run();
            else
                Post(context, runnable);
        }
    }

    public static void Post(Context context, Runnable runnable)
    {
        new Handler(context.getMainLooper()).post(runnable);
    }

    public static List<String> LsAssets(Context context, String assetFolderPath)
    {
        List<String> subList = new ArrayList<>();
        if(LsAssets_r(context, assetFolderPath, "", subList))
            return subList;
        else
            return null;
    }

    private static boolean LsAssets_r(Context context, String assetFolderPath, String prefix, List<String> res)
    {
        try
        {
            String[] list = context.getAssets().list(assetFolderPath);
            if(null == list || list.length == 0)
            {
                return false;
            }
            for(String str : list)
            {
                String path = KStr.AppendPath(assetFolderPath, str);
                String subPrefix = KStr.IsBlank(prefix) ? str : KStr.AppendPath(prefix, str);
                List<String> subList = new ArrayList<>();
                if(LsAssets_r(context, path, subPrefix, subList))
                {
                    res.addAll(subList);
                }
                else
                {
                    res.add(subPrefix);
                }
            }
            return true;
        }
        catch (Exception e)
        {
            return false;
        }
    }

    public static int[] GetGeometry(Activity context, boolean currentIsLandscape, boolean hideNav, boolean coverEdges)
    {
        int safeInsetTop = Q3EUtils.GetEdgeHeight(context, currentIsLandscape);
        int safeInsetBottom = Q3EUtils.GetEndEdgeHeight(context, currentIsLandscape);
        // if invert
        if(Q3EUtils.ActiveIsInvert(context))
        {
            int tmp = safeInsetTop;
            safeInsetTop = safeInsetBottom;
            safeInsetBottom = tmp;
        }
        int[] fullSize = Q3EUtils.GetFullScreenSize(context);
        int[] size = Q3EUtils.GetNormalScreenSize(context);
        if(currentIsLandscape)
        {
            int tmp = fullSize[0];
            fullSize[0] = fullSize[1];
            fullSize[1] = tmp;

            tmp = size[0];
            size[0] = size[1];
            size[1] = tmp;
        }
        int navBarHeight = fullSize[1] - size[1] - safeInsetTop - safeInsetBottom;
        int start = safeInsetTop;
        int w = fullSize[0];
        int h = fullSize[1];
        if (!hideNav)
            h -= navBarHeight;
        if (!coverEdges)
            h -= (safeInsetTop + safeInsetBottom);

        final int Width = Math.max(w, h);
        final int Height = Math.min(w, h);

        return currentIsLandscape ? new int[] { start, 0, Width, Height, } : new int[] { 0, start, Height, Width, };
    }

    public static List<String> FilesExistsInDirectory(String dir, String...files)
    {
        List<String> res = new ArrayList<>();
        for(String file : files)
        {
            File f = new File(KStr.AppendPath(dir, file));
            if(!f.exists())
                res.add(file);
        }
        return res;
    }

    public static String asset_get_contents(Context context, String path)
    {
        InputStream is = null;
        try
        {
            is = context.getAssets().open(path);
            String res = Read(is);
            return res;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return null;
        }
        finally
        {
            Close(is);
        }
    }

    public static void SetViewZ(View view, int z)
    {
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
            view.setZ(z);
    }

    public static boolean ContainsIgnoreCase(Collection<String> list, String target)
    {
        for(String s : list)
        {
            if(target.equalsIgnoreCase(s))
                return true;
        }
        return false;
    }

    public static int ArrayIndexOf(String[] arr, String target, boolean cs)
    {
        for(int i = 0; i < arr.length; i++)
        {
            if(cs)
            {
                if(target.equals(arr[i]))
                    return i;
            }
            else
            {
                if(target.equalsIgnoreCase(arr[i]))
                    return i;
            }
        }
        return -1;
    }

    public static int[] CalcSizeByRatio(int screenWidth, int screenHeight, int ratioWidth, int ratioHeight)
    {
        int w, h;
        int x = 0, y = 0;
        int mask = 0;
        if (ratioWidth <= 0 || ratioHeight <= 0)
        {
            w = screenWidth;
            h = screenHeight;
        }
        else
        {
            BigDecimal targetRatio = new BigDecimal(ratioWidth).divide(new BigDecimal(ratioHeight), 6, RoundingMode.DOWN);
            BigDecimal screenRatio = new BigDecimal(screenWidth).divide(new BigDecimal(screenHeight), 6, RoundingMode.DOWN);
            int cmp = targetRatio.compareTo(screenRatio);
            if(cmp > 0)
            {
                h = (int) ((float) screenWidth * ((float)ratioHeight / (float)ratioWidth));
                mask |= 2;
                w = screenWidth;
                y = (screenHeight - h) / 2;
            }
            else if(cmp < 0)
            {
                w = (int) ((float) screenHeight * ((float)ratioWidth / (float)ratioHeight));
                mask |= 1;
                h = screenHeight;
                x = (screenWidth - w) / 2;
            }
            else
            {
                w = screenWidth;
                h = screenHeight;
            }
        }
        return new int[]{x, y, w, h, mask};
    }
}
