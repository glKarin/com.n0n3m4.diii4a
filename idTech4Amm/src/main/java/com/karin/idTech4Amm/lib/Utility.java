package com.karin.idTech4Amm.lib;

import com.n0n3m4.q3e.Q3EUtils;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.List;

/**
 * Common utility
 */
public final class Utility
{
    private Utility() {}
    
    public static boolean MASK(int a, int b)
    {
        return (a & b) != 0;
    }
    
    public static int BIT(int a, int b)
    {
        return a << b;
    }

    public static int ArrayIndexOf(int[] arr, int target)
    {
        for(int i = 0; i < arr.length; i++)
        {
            if(arr[i] == target)
                return i;
        }
        return 0;
    }

    public static int ArrayIndexOf(Object[] arr, Object target)
    {
        for(int i = 0; i < arr.length; i++)
        {
            if(target.equals(arr[i]))
                return i;
        }
        return -1;
    }

    public static boolean ArrayContains(Object[] arr, Object target)
    {
        return ArrayIndexOf(arr, target) >= 0;
    }

    public static boolean ArrayContains(String[] arr, String target, boolean cs)
    {
        return Q3EUtils.ArrayIndexOf(arr, target, cs) >= 0;
    }

    public static boolean InArrayRange(Object[] arr, int index)
    {
        return index >= 0 && index <= arr.length;
    }

    public static int IndexOf(List<String> arr, String target, boolean cs)
    {
        for(int i = 0; i < arr.size(); i++)
        {
            if(cs)
            {
                if(target.equals(arr.get(i)))
                    return i;
            }
            else
            {
                if(target.equalsIgnoreCase(arr.get(i)))
                    return i;
            }
        }
        return -1;
    }

    public static boolean Contains(List<String> arr, String target, boolean cs)
    {
        return IndexOf(arr, target, cs) >= 0;
    }

    public static int Step(int a, int step)
    {
        return (int)Math.round((float)a / (float)step) * step;
    }

    public static String Hex(byte[] bytes)
    {
        StringBuilder buf = new StringBuilder();

        for(byte b : bytes)
        {
            String hex = Integer.toString(Byte.toUnsignedInt(b) & 0xFF, 16).toLowerCase();
            if(hex.length() < 2)
                buf.append("0");
            buf.append(hex);
        }

        return buf.toString();
    }

    public static String MD5(String str)
    {
        try
        {
            byte[] bytes = MessageDigest.getInstance("MD5").digest(str.getBytes(StandardCharsets.UTF_8));
            return Hex(bytes);
        }
        catch(NoSuchAlgorithmException e)
        {
            e.printStackTrace();
            return null;
        }
    }
}
