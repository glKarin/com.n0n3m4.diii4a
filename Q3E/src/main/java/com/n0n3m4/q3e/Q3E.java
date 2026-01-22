package com.n0n3m4.q3e;

import android.view.Surface;
import android.util.Log;
import android.os.Build;

import com.n0n3m4.q3e.device.Q3EVirtualMouse;
import com.n0n3m4.q3e.event.Q3EExitEvent;
import com.n0n3m4.q3e.event.Q3EQuitEvent;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KOnceRunnable;
import com.n0n3m4.q3e.karin.KStr;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class Q3E
{
    public static Q3EMain       activity;
    public static Surface       surface;
    public static Q3EGameThread gameThread;

    public static          Q3EView        gameView;
    public static          Q3EControlView controlView;
    public static volatile boolean         running = false;
    public static          Q3EVirtualMouse virtualMouse;


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


    public static void runOnUiThread(Runnable action) {
        if(null != activity)
            activity.runOnUiThread(action);
    }

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
        if(Q3E.GAME_VIEW_WIDTH == Q3E.surfaceWidth)
            Q3E.widthRatio = 1.0f;
        else
            Q3E.widthRatio = (float) Q3E.GAME_VIEW_WIDTH / (float) Q3E.surfaceWidth;
        if(Q3E.GAME_VIEW_HEIGHT == Q3E.surfaceHeight)
            Q3E.heightRatio = 1.0f;
        else
            Q3E.heightRatio = (float) Q3E.GAME_VIEW_HEIGHT / (float) Q3E.surfaceHeight;
        KLog.i("Q3EView", "Surface: view physical size=%d x %d, game logical size=%d x %d, ratio=%f, %f", Q3E.GAME_VIEW_WIDTH, Q3E.GAME_VIEW_HEIGHT, Q3E.surfaceWidth, Q3E.surfaceHeight, Q3E.widthRatio, Q3E.heightRatio);
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
        int gameThread = Q3EPreference.GetIntFromString(activity, Q3EPreference.GAME_THREAD, Q3EGlobals.GAME_THREAD_TYPE_NATIVE);
        int threadStackSize = Q3EPreference.GetIntFromString(activity, Q3EPreference.GAME_THREAD_STACK_SIZE, 0);
        Q3E.gameThread = gameThread == Q3EGlobals.GAME_THREAD_TYPE_JAVA
                ? (threadStackSize > 0 ? new Q3EGameThreadJava(Q3EJNI.AlignedStackSize(threadStackSize)) : new Q3EGameThreadJava() )
        : new Q3EGameThreadNative();
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


    public static String GetDLLCachePath(String dir)
    {
        String path = activity.getCacheDir().getAbsolutePath();// /data/user/<package_name>/cache/
        if(KStr.NotEmpty(dir))
            path = KStr.AppendPath(path, dir);
        return path;
    }

    // 1: fileName
    // 2: fileName.so
    // 3: libfileName.so
    public static String CopyDLLToCache(String dllDir, String fileName, String subDir, String name)
    {
        List<String> guess = new ArrayList<>();
        guess.add(fileName);
        String filename = fileName.toLowerCase();
        if(!filename.endsWith(".so"))
        {
            fileName += ".so";
            guess.add(fileName);
        }
        if(!filename.startsWith("lib"))
            guess.add("lib" + fileName);

        for(String g : guess)
        {
            String res = CopyDLLToCache(KStr.AppendPath(dllDir, g), subDir, name);
            if(null != res)
                return res;
        }
        return null;
    }

    public static String CopyDLLToCache(String dllPath, String subDir, String name)
    {
        File file = new File(dllPath);
        if(!file.isFile() || !file.canRead())
        {
            Log.w(Q3EGlobals.CONST_Q3E_LOG_TAG, "Missing DLL " + dllPath);
            return null;
        }

        String targetDir = GetDLLCachePath(subDir); // /data/user/<package_name>/cache/
        Q3EUtils.mkdir(targetDir, true);
        if(KStr.IsEmpty(name))
            name = file.getName();
        String cacheFile = KStr.AppendPath(targetDir, name);
        String res = null;

        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copy DLL " + dllPath + " -> " + cacheFile);
        long r = Q3EUtils.cp(dllPath, cacheFile);
        if(r > 0)
        {
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copy DLL success: " + dllPath + " -> " + cacheFile);
            res = cacheFile;
        }
        else
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copy DLL fail: " + dllPath + " -> " + cacheFile);
        return res;
    }

    public static String CopyDLLsToCache(String dllDirPath, String subDir, String...excludes)
    {
        File dir = new File(dllDirPath);
        if(!dir.isDirectory())
            return null;

        File[] files = dir.listFiles();
        if(null == files || files.length == 0)
            return null;

        List<String> excludeList = null;
        if(null != excludes && excludes.length > 0)
        {
            excludeList = Arrays.asList(excludes);
        }
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copy DLLs " + dllDirPath);
        for(File file : files)
        {
            if(!file.getName().toLowerCase().endsWith(".so"))
                continue;
            if(null != excludeList && Q3EUtils.ContainsIgnoreCase(excludeList, file.getName()))
                continue;
            CopyDLLToCache(file.getAbsolutePath(), subDir, file.getName());
        }
        return GetDLLCachePath(subDir);
    }
}
