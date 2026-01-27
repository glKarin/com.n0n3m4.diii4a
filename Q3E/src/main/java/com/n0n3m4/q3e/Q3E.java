package com.n0n3m4.q3e;

import android.content.Context;
import android.view.Surface;
import android.util.Log;
import android.os.Build;
import android.view.View;
import android.view.inputmethod.InputMethodManager;

import com.n0n3m4.q3e.device.Q3EOuya;
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
    private static final String TAG = "Q3E";

    public static Q3EMain       activity;
    public static Surface       surface;
    public static Q3EGameThread gameThread;

    public static Q3ECallbackObj callbackObj;
    public static Q3EEventEngine eventEngine = new Q3EEventEngineJava();
    public static Q3EInterface q3ei = new Q3EInterface(); //k: new
    public static boolean isOuya = false;
    public static Q3EKeyboard keyboard = null;

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

    public static boolean joystick_smooth          = true; // Q3EView::analog
    public static boolean function_key_toolbar = false;
    public static boolean builtin_virtual_keyboard = false;
    public static boolean m_usingMouse = false;
    public static int supportDevices = 0;

    public static void runOnUiThread(Runnable action) {
        if(null != activity)
            activity.runOnUiThread(action);
    }

    public static void post(Runnable action) {
        controlView.post(action);
    }

    public static int LogicalToPhysicsX(int x)
    {
        return GAME_VIEW_WIDTH == surfaceWidth ? x : (int) ((float) x * widthRatio);
    }

    public static int LogicalToPhysicsY(int y)
    {
        return GAME_VIEW_HEIGHT == surfaceHeight ? y : (int) ((float) y * heightRatio);
    }

    public static float PhysicsToLogicalX(float x)
    {
        return GAME_VIEW_WIDTH == surfaceWidth ? x : (x / widthRatio);
    }

    public static float PhysicsToLogicalY(float y)
    {
        return GAME_VIEW_HEIGHT == surfaceHeight ? y : (y / heightRatio);
    }

    public static boolean IsOriginalSize()
    {
        return GAME_VIEW_WIDTH == surfaceWidth && GAME_VIEW_HEIGHT == surfaceHeight;
    }

    public synchronized static void CalcRatio()
    {
        if(GAME_VIEW_WIDTH == surfaceWidth)
            widthRatio = 1.0f;
        else
            widthRatio = (float) GAME_VIEW_WIDTH / (float) surfaceWidth;
        if(GAME_VIEW_HEIGHT == surfaceHeight)
            heightRatio = 1.0f;
        else
            heightRatio = (float) GAME_VIEW_HEIGHT / (float) surfaceHeight;
        KLog.i("Q3EView", "Surface: view physical size=%d x %d, game logical size=%d x %d, ratio=%f, %f", GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT, surfaceWidth, surfaceHeight, widthRatio, heightRatio);
    }

    public static void sendAnalog(boolean down, float x, float y)
    {
        eventEngine.SendAnalogEvent(down, x, y);
    }

    public static void sendKeyEvent(boolean down, int keycode, int charcode)
    {
        eventEngine.SendKeyEvent(down, keycode, charcode);
    }

    public static void sendMotionEvent(float deltax, float deltay)
    {
        eventEngine.SendMotionEvent(deltax, deltay);
    }

    public static void sendMouseEvent(float x, float y/*, boolean relativeMode*/)
    {
        eventEngine.SendMouseEvent(x, y, false);
    }

    public static void sendTextEvent(String text)
    {
        eventEngine.SendTextEvent(text);
    }

    public static void sendCharEvent(int ch)
    {
        eventEngine.SendCharEvent(ch);
    }

    public static void sendWheelEvent(float x, float y)
    {
        eventEngine.SendWheelEvent(x, y);
    }

    public static void sendAnalogDelayed(boolean down, float x, float y, View view, int delay)
    {
        view.postDelayed(new Runnable() {
            @Override
            public void run() {
                eventEngine.SendAnalogEvent(down, x, y);
            }
        }, delay);
    }

    public static void sendKeyEventDelayed(boolean down, int keycode, int charcode, View view, int delay)
    {
        view.postDelayed(new Runnable() {
            @Override
            public void run() {
                eventEngine.SendKeyEvent(down, keycode, charcode);
            }
        }, delay);
    }

    public static void sendMotionEventDelayed(float deltax, float deltay, View view, int delay)
    {
        view.postDelayed(new Runnable() {
            @Override
            public void run() {
                eventEngine.SendMotionEvent(deltax, deltay);
            }
        }, delay);
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
        int gameThreadImpl = Q3EPreference.GetIntFromString(activity, Q3EPreference.GAME_THREAD, Q3EGlobals.GAME_THREAD_TYPE_NATIVE);
        int threadStackSize = Q3EPreference.GetIntFromString(activity, Q3EPreference.GAME_THREAD_STACK_SIZE, 0);
        gameThread = gameThreadImpl == Q3EGlobals.GAME_THREAD_TYPE_JAVA
                ? (threadStackSize > 0 ? new Q3EGameThreadJava(Q3EJNI.AlignedStackSize(threadStackSize)) : new Q3EGameThreadJava() )
        : new Q3EGameThreadNative();
        gameThread.Start();
        running = true;
    }

    // activity onDestroyed
    public static void Stop()
    {
        if(running)
        {
            running = false;
            new Q3EExitEvent().run();
            Q3EGameThread.Sleep(100);
            if(null != gameThread)
            {
                gameThread.Stop();
                gameThread = null;
            }
            Q3EGameThread.Sleep(100);

            if(null != callbackObj)
                callbackObj.OnDestroy();
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
        if(running)
        {
            running = false;
            callbackObj.PushEvent(new Q3EQuitEvent());
            Q3EGameThread.Sleep(100);
            if(null != gameThread)
            {
                gameThread.Stop();
                gameThread = null;
            }
            Q3EGameThread.Sleep(100);
        }
        Finish();
    }

    public static void Pause()
    {
        if(!running)
            return;
        Runnable runnable = new KOnceRunnable() {
            @Override public void Run() {
                Q3EJNI.OnPause();
            }
        };
        callbackObj.PushEvent(runnable);
    }

    public static void Resume()
    {
        if(!running)
            return;
        Runnable runnable = new KOnceRunnable() {
            @Override public void Run() {
                Q3EJNI.OnResume();
            }
        };
        callbackObj.PushEvent(runnable);
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

    public static void SetupEventEngine(Context context)
    {
        int eventQueue = Q3EPreference.GetIntFromString(context, Q3EPreference.EVENT_QUEUE, Q3EGlobals.EVENT_QUEUE_TYPE_NATIVE);
        if(eventQueue == Q3EGlobals.EVENT_QUEUE_TYPE_JAVA)
        {
            Log.i(TAG, "Using java event queue");
            eventEngine = new Q3EEventEngineJava();
        }
        else
        {
            Log.i(TAG, "Using native event queue");
            eventEngine = new Q3EEventEngineNative();
        }

        if(q3ei.IsUsingSDL())
        {
            Log.i(TAG, "Using SDL event queue");
            eventEngine = new Q3EEventEngineSDL(eventEngine);
        }
        else if(q3ei.IsUsingVirtualMouse())
        {
            Log.i(TAG, "Using Sam event queue");
            eventEngine = new Q3EEventEngineSam(eventEngine);
        }
    }

    public static void togglevkbd()
    {
        InputMethodManager imm = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (function_key_toolbar)
        {
            boolean changed;
            if(builtin_virtual_keyboard)
            {
                keyboard.ToggleBuiltInVKB();
                changed = keyboard.IsBuiltInVKBVisible();
                ToggleToolbar(changed);
            }
            else
            {
                changed = imm.hideSoftInputFromWindow(controlView.getWindowToken(), 0);
                if (changed) // im from open to close
                    ToggleToolbar(false);
                else // im is closed
                {
                    //imm.showSoftInput(vw, InputMethodManager.SHOW_FORCED);
                    imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
                    ToggleToolbar(true);
                }
            }
        }
        else
        {
            if(builtin_virtual_keyboard)
                keyboard.ToggleBuiltInVKB();
            else
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
        }
    }

    public static void ToggleToolbar(boolean on)
    {
        callbackObj.ToggleToolbar(on);
    }

    public static void OpenVKB()
    {
        if (null != controlView)
        {
            InputMethodManager imm = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
            if(builtin_virtual_keyboard)
                keyboard.OpenBuiltInVKB();
            else
            {
                //imm.showSoftInput(vw, InputMethodManager.SHOW_FORCED);
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
            }
        }
        if (function_key_toolbar)
            ToggleToolbar(true);
    }

    public static void CloseVKB()
    {
        if (null != controlView)
        {
            if(builtin_virtual_keyboard)
            {
                keyboard.CloseBuiltInVKB();
            }
            else
            {
                InputMethodManager imm = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(controlView.getWindowToken(), 0);
            }
        }
        if (function_key_toolbar)
            ToggleToolbar(false);
    }

    public static String GetDataPath(String filename)
    {
        String path = "";
        if(null != q3ei.datadir)
            path += q3ei.datadir;
        if(KStr.NotEmpty(filename))
            path += filename;
        return path;
    }

    static
    {
        isOuya = Q3EOuya.IsValid();
    }
}
