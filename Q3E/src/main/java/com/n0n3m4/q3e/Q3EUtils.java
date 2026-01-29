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

import android.os.Build;
import android.view.InputDevice;
import android.view.View;

import com.n0n3m4.q3e.device.Q3EMouseDevice;
import com.n0n3m4.q3e.karin.KLog;
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
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.List;

public final class Q3EUtils
{
    private Q3EUtils() {}

    public static int nextpowerof2(int x)
    {
        int candidate = 1;
        while (candidate < x)
            candidate *= 2;
        return candidate;
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
            Close(os);
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
            return Copy(os, is);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return -1;
        }
        finally
        {
            Close(is);
            Close(os);
        }
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
            Close(fileoutputstream);
        }
    }

    public static long Write(String filePath, InputStream in, int...bufferSizeArg) throws RuntimeException
    {
        FileOutputStream fileoutputstream = null;

        try
        {
            fileoutputstream = new FileOutputStream(filePath);
            return Copy(fileoutputstream, in, bufferSizeArg);
        }
        catch(FileNotFoundException e)
        {
            throw new RuntimeException(e);
        }
        finally
        {
            Close(fileoutputstream);
        }
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

    public static boolean HasMouseDevice()
    {
        int[] deviceIds = InputDevice.getDeviceIds();
        for(int deviceId : deviceIds)
        {
            InputDevice device = InputDevice.getDevice(deviceId);

            if(null == device)
                continue;
            if ((device.getSources() & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE)
                return true;
        }
        return false;
    }

    public static boolean IncludeBit(int a, int b)
    {
        return (a & b) == b;
    }

    public static long parseLong_s(String str, long...def)
    {
        long defVal = null != def && def.length > 0 ? def[0] : 0;
        if(null == str)
            return defVal;
        str = str.trim();
        if(str.isEmpty())
            return defVal;
        try
        {
            return Long.parseLong(str);
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return defVal;
        }
    }

    public static String toFixed(double d, int num)
    {
        StringBuilder pattern = new StringBuilder("0");
        if (num > 0) {
            pattern.append(".");
            for(int i = 0; i < num; i++)
            {
                pattern.append('0');
            }
        }

        DecimalFormat df = new DecimalFormat(pattern.toString());
        df.setRoundingMode(RoundingMode.HALF_UP);
        return df.format(d);
    }
}
