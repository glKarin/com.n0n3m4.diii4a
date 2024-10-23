package com.karin.idTech4Amm.lib;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

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
