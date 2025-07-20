package com.n0n3m4.q3e;

import android.view.Surface;
import android.util.Log;
import android.os.Build;

import com.n0n3m4.q3e.event.Q3EExitEvent;
import com.n0n3m4.q3e.event.Q3EQuitEvent;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public final class Q3E
{
    public static Q3EMain       activity;
    public static Surface       surface;
    public static Q3EGameThread gameThread;

    public static          Q3EView        gameView;
    public static          Q3EControlView controlView;
    public static volatile boolean        running = false;


    // surface size
    public static int surfaceWidth = 1;
    public static int surfaceHeight = 1;
    // window size(control view)
    public static int orig_width = 1;
    public static int orig_height = 1;
    // window size(game view)
    public static int GAME_VIEW_WIDTH = 1;
    public static int GAME_VIEW_HEIGHT = 1;
    // ratio
    public static float widthRatio = 1.0f;
    public static float heightRatio = 1.0f;

    public static int LogicalToPhysicsX(int x)
    {
        return Q3E.GAME_VIEW_WIDTH == Q3E.surfaceWidth ? x : (int) ((float) x * Q3E.widthRatio);
    }

    public static int LogicalToPhysicsY(int y)
    {
        return Q3E.GAME_VIEW_HEIGHT == Q3E.surfaceHeight ? y : (int) ((float) y * Q3E.heightRatio);
    }

    public static float PhysicsToLogicalX(float x)
    {
        return Q3E.GAME_VIEW_WIDTH == Q3E.surfaceWidth ? x : (x / Q3E.widthRatio);
    }

    public static float PhysicsToLogicalY(float y)
    {
        return Q3E.GAME_VIEW_HEIGHT == Q3E.surfaceHeight ? y : (y / Q3E.heightRatio);
    }

    public static boolean IsOriginalSize()
    {
        return Q3E.GAME_VIEW_WIDTH == Q3E.surfaceWidth && Q3E.GAME_VIEW_HEIGHT == Q3E.surfaceHeight;
    }

    public synchronized static void CalcRatio()
    {
        Q3E.widthRatio = (float) Q3E.GAME_VIEW_WIDTH / (float) Q3E.surfaceWidth;
        Q3E.heightRatio = (float) Q3E.GAME_VIEW_HEIGHT / (float) Q3E.surfaceHeight;
    }

    public static void Finish()
    {
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Finish activity......");
        if(null != activity && !activity.isFinishing())
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
                activity.finishAffinity();
            else
                activity.finish();
        }
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Finish activity done");
        System.exit(0);
    }

    public static void Start()
    {
        int gameThread = Q3EPreference.GetIntFromString(activity, Q3EPreference.GAME_THREAD, 0);
        Q3E.gameThread = gameThread == 1 ? new Q3EGameThreadJava() : new Q3EGameThreadNative();
        Q3E.gameThread.Start();
        Q3E.running = true;
    }

    // activity onDestroyed
    public static void Stop()
    {
        if(Q3E.running)
        {
            Q3E.running = false;
            new Q3EExitEvent().run();
            Q3EGameThread.Sleep(100);
            if(null != Q3E.gameThread)
            {
                Q3E.gameThread.Stop();
                Q3E.gameThread = null;
            }
            Q3EGameThread.Sleep(100);
        }
    }

    // press back button
    public static void Shutdown()
    {
/*        Q3EUtils.q3ei.callbackObj.PushEvent(new Runnable() {
            public void run()
            {
                if(mInit)
                    Q3EJNI.shutdown();
                if(null != andThen)
                    andThen.run();
            }
        });*/
        if(Q3E.running)
        {
            Q3E.running = false;
            Q3EUtils.q3ei.callbackObj.PushEvent(new Q3EQuitEvent());
            Q3EGameThread.Sleep(100);
            if(null != Q3E.gameThread)
            {
                Q3E.gameThread.Stop();
                Q3E.gameThread = null;
            }
            Q3EGameThread.Sleep(100);
        }
        Finish();
    }

    public static void Pause()
    {
        if(!Q3E.running)
            return;
        Runnable runnable = new KOnceRunnable() {
            @Override public void Run() {
                Q3EJNI.OnPause();
            }
        };
        Q3EUtils.q3ei.callbackObj.PushEvent(runnable);
    }

    public static void Resume()
    {
        if(!Q3E.running)
            return;
        Runnable runnable = new KOnceRunnable() {
            @Override public void Run() {
                Q3EJNI.OnResume();
            }
        };
        Q3EUtils.q3ei.callbackObj.PushEvent(runnable);
    }
}
