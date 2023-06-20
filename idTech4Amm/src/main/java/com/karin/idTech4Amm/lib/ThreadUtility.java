package com.karin.idTech4Amm.lib;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;

public final class ThreadUtility
{
    public static void Post(String name, Runnable runnable, Runnable andThen)
    {
        HandlerThread handlerThread = new HandlerThread(name);
        handlerThread.start();
        Handler handler = new Handler(handlerThread.getLooper());
        Looper looper = Looper.myLooper();
        handler.post(new Runnable()
        {
            @Override
            public void run()
            {
                runnable.run();
                new Handler(looper).post(new Runnable()
                {
                    @Override
                    public void run()
                    {
                        handlerThread.quit();
                        if(null != andThen)
                            andThen.run();
                    }
                });
            }
        });
    }

    public static void Post(Runnable runnable, Runnable andThen)
    {
        Post("ThreadUtility", runnable, andThen);
    }

    public static void Post(String name, Runnable runnable)
    {
        Post("ThreadUtility", runnable, null);
    }

    public static void Post(Runnable runnable)
    {
        Post(runnable, null);
    }

    private ThreadUtility() {}
}
