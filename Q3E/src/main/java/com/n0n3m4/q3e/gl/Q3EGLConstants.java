package com.n0n3m4.q3e.gl;

import android.os.Build;

public final class Q3EGLConstants
{
    public static final int OPENGLES20 = 0x00020000;
    public static final int OPENGLES30 = 0x00030000;


    public static int GetPreferOpenGLESVersion()
    {
        return /*IsSupportOpenGLES3() ? OPENGLES30 : */OPENGLES20;
    }

    public static boolean IsSupportOpenGLES3()
    {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2); // Android 4.3 jelly bean
    }

    private Q3EGLConstants() {}
}
