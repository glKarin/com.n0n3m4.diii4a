package com.n0n3m4.q3e;

import com.n0n3m4.q3e.karin.KLog;

public interface Q3EGameThread
{
    public void Start();
    public void Stop();
    public boolean IsRunning();

    public static void Sleep(int ms)
    {
        try
        {
            Thread.sleep(ms);
        }
        catch(InterruptedException e)
        {
            e.printStackTrace();
        }
    }
}

class Q3EGameThreadJava extends Thread implements Q3EGameThread
{
    public Q3EGameThreadJava()
    {
        super("Q3EMainJava");

        setPriority(MAX_PRIORITY);
    }

    public Q3EGameThreadJava(int stackSize)
    {
        super(null, null, "Q3EMainJava", stackSize);

        setPriority(MAX_PRIORITY);
    }

    private void Log(String str, Object...args)
    {
        KLog.i("Q3EGameThreadJava", str, args);
        System.out.printf((str) + "%n", args);
    }

    public synchronized void Start()
    {
        start();
    }

    @Override
    public void run()
    {
        Log("Q3EGameThread start running......");
        int resultCode = Q3EJNI.main();
        Log("Q3EGameThread end: " + resultCode);

        Q3E.running = false;
        if(null != Q3E.activity && !Q3E.activity.isFinishing())
        {
            Q3E.activity.runOnUiThread(new Runnable() {
                @Override
                public void run()
                {
                    Q3E.Finish();
                }
            });
        }
    }

    public synchronized void Stop()
    {
        if(!isAlive())
            return;
        try
        {
            join(1000);
            if(!isAlive())
                return;
        }
        catch(InterruptedException e)
        {
            e.printStackTrace();
        }
        if(!isInterrupted())
            interrupt();
    }

    @Override
    public boolean IsRunning()
    {
        return isAlive()
                //&& !isInterrupted()
                ;
    }
}

class Q3EGameThreadNative implements Q3EGameThread
{
    private boolean running = false;

    public Q3EGameThreadNative()
    {
    }

    private void Log(String str, Object...args)
    {
        KLog.i("Q3EGameThreadNative", str, args);
    }

    public synchronized void Start()
    {
        if(running)
            return;

        Log("Q3EGameThread start native thread......");
        Q3EJNI.StartThread();
        running = true;
        Log("Q3EGameThread native thread started");
    }

    public synchronized void Stop()
    {
        if(!running)
            return;

        running = false;
        Log("Q3EGameThread stop native thread......");
        Q3EJNI.StopThread();
        Log("Q3EGameThread native thread stopped");
    }

    public boolean IsRunning()
    {
        return running;
    }
}
