package com.karin.idTech4Amm.lib;

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
    
    public static float parseFloat_s(String str, float...def)
    {
        float defVal = def.length > 0 ? def[0] : 0.0f;
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
}
