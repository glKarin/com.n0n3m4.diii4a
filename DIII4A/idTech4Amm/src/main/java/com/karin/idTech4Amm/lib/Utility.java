package com.karin.idTech4Amm.lib;

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
}
