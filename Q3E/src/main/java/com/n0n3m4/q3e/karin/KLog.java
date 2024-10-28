package com.n0n3m4.q3e.karin;

import android.util.Log;

import com.n0n3m4.q3e.Q3EGlobals;

// Log utils
public class KLog
{
    public static String Str(Object fmt, Object...args)
    {
        if(null == fmt)
            return "(null)";
        if(fmt instanceof String)
            return String.format((String)fmt, args);
        else
            return fmt.toString();
    }

    public static void v(String tag, Object fmt, Object...args)
    {
        String str = Str(fmt, args);
        Log.v(tag, str);
    }

    public static void d(String tag, Object fmt, Object...args)
    {
        String str = Str(fmt, args);
        Log.d(tag, str);
    }

    public static void i(String tag, Object fmt, Object...args)
    {
        String str = Str(fmt, args);
        Log.i(tag, str);
    }

    public static void w(String tag, Object fmt, Object...args)
    {
        String str = Str(fmt, args);
        Log.w(tag, str);
    }

    public static void e(String tag, Object fmt, Object...args)
    {
        String str = Str(fmt, args);
        Log.e(tag, str);
    }

    public static void V(Object fmt, Object...args)
    {
        v(Q3EGlobals.CONST_Q3E_LOG_TAG, fmt, args);
    }

    public static void D(Object fmt, Object...args)
    {
        d(Q3EGlobals.CONST_Q3E_LOG_TAG, fmt, args);
    }

    public static void I(Object fmt, Object...args)
    {
        i(Q3EGlobals.CONST_Q3E_LOG_TAG, fmt, args);
    }

    public static void W(Object fmt, Object...args)
    {
        w(Q3EGlobals.CONST_Q3E_LOG_TAG, fmt, args);
    }

    public static void E(Object fmt, Object...args)
    {
        e(Q3EGlobals.CONST_Q3E_LOG_TAG, fmt, args);
    }
	
	private KLog() {}
}
