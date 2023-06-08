package com.n0n3m4.q3e.tv;

import android.content.Context;

import java.lang.reflect.Method;

public final class Q3EOuya
{
    public static final int BUTTON_L3 = 106; // OuyaController.BUTTON_L3
    public static final int BUTTON_R3 = 107; // OuyaController.BUTTON_R3

    public static boolean IsValid()
    {
        return "cardhu".equals(android.os.Build.DEVICE) || android.os.Build.DEVICE.contains("ouya");//Dunno wtf is cardhu
    }

    public static boolean Init(Context context)
    {
        if(!IsValid())
            return false;

        try
        {
            Class<?> clazz = Class.forName("tv.ouya.console.api.OuyaController");
            Method init = clazz.getDeclaredMethod("init", android.content.Context.class);
            init.invoke(null, context);
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
    }
}
