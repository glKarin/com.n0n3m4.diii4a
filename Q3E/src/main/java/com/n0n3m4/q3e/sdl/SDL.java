package com.n0n3m4.q3e.sdl;

import android.content.Context;

import java.lang.Class;
import java.lang.reflect.Method;

/**
    SDL library initialization
*/
public class SDL {

    // This function should be called first and sets up the native code
    // so it can call into the Java classes
    public static void setupJNI() {
//        SDLActivity.nativeSetupJNI();
//        SDLControllerManager.nativeSetupJNI();
    }

    // This function should be called each time the activity is started
    public static void initialize() {
        setContext(null);

        SDLActivity.initialize();
        SDLControllerManager.initialize();
    }

    // This function stores the current activity (SDL or not)
    public static void setContext(Context context) {
        mContext = context;
    }

    public static Context getContext() {
        return mContext;
    }

    protected static Context mContext;
}
