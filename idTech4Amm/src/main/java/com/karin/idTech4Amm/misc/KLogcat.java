package com.karin.idTech4Amm.misc;

import android.os.Build;
import android.util.Log;

import com.n0n3m4.q3e.Q3EUtils;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public final class KLogcat
{
    private KLogcatThread m_thread;
    private String m_command = "logcat";

    public void SetCommand(String str)
    {
        m_command = str;
    }

    public void Start(KLogcatCallback callback)
    {
        Stop();
        m_thread = new KLogcatThread();
        m_thread.SetCallback(callback);
        m_thread.Start(m_command);
    }

    public void Stop()
    {
        if(null != m_thread)
        {
            m_thread.Stop();
            m_thread = null;
        }
    }

    public void Pause()
    {
        if(null != m_thread)
        {
            m_thread.Pause();
        }
    }

    public void Resume()
    {
        if(null != m_thread)
        {
            m_thread.Resume();
        }
    }

    public boolean IsRunning()
    {
        return null != m_thread && m_thread.m_running;
    }

    public boolean IsPaused()
    {
        return IsRunning() && m_thread.m_paused;
    }

    public interface KLogcatCallback
    {
        public void Output(String str);
    }

    private static class KLogcatThread extends Thread
    {
        private static final String TAG     = "KLogcat";
        private static final int    WAIT_MS = 1000;

        private volatile boolean m_running = false;
        private volatile boolean m_paused  = false;
        private KLogcatCallback m_callback;
        private String m_command = "logcat";
        private final Object m_locker = new Object();

        public KLogcatThread()
        {
            setName("KLogcatThread");
        }

        @Override
        public void start()
        {
            if(m_running)
                return;
            m_running = true;
            m_paused = false;
            Log.i(TAG, Thread.currentThread().getName() + ": Requesting start thread......");
            super.start();
            Log.i(TAG, Thread.currentThread().getName() + ": Requesting start done");
        }

        public void Start(String cmd)
        {
            if(m_running)
                Stop();
            if(null != cmd && !cmd.isEmpty())
                m_command = cmd;
            this.start();
        }

        public void Stop()
        {
            Log.i(TAG, Thread.currentThread().getName() + ": Requesting stop thread......");
            m_running = false;
            synchronized(m_locker) {
                try
                {
                    if(m_paused)
                    {
                        Log.i(TAG, Thread.currentThread().getName() + ": Awaiting thread resume......");
                        try
                        {
                            Thread.sleep(WAIT_MS + 10);
                        }
                        catch(InterruptedException e)
                        {
                            e.printStackTrace();
                        }
                        m_paused = false;
                        m_locker.notifyAll();
                    }
                }
                catch(Exception e)
                {
                    // e.printStackTrace();
                }
            }
            interrupt();
            Log.i(TAG, Thread.currentThread().getName() + ": Requesting stop done");
        }

        public void Pause()
        {
            if(!m_running)
                return;
            if(m_paused)
                return;
            Log.i(TAG, Thread.currentThread().getName() + ": Requesting pause thread......");
            m_paused = true;
            try
            {
                Thread.sleep(WAIT_MS + 10);
            }
            catch(InterruptedException e)
            {
                e.printStackTrace();
            }
            Log.i(TAG, Thread.currentThread().getName() + ": Requesting pause thread done");
        }

        public void Resume()
        {
            if(!m_running)
                return;
            if(!m_paused)
                return;
            Log.i(TAG, Thread.currentThread().getName() + ": Requesting resume thread......");
            synchronized(m_locker) {
                try
                {
                    m_paused = false;
                    m_locker.notifyAll();
                }
                catch(Exception e)
                {
                    e.printStackTrace();
                }
            }
            Log.i(TAG, Thread.currentThread().getName() + ": Requesting resume thread done");
        }

        public void SetCallback(KLogcatCallback callback)
        {
            m_callback = callback;
        }

        @Override
        public void run()
        {
            synchronized(m_locker) {
                int sleepCount = 0;
                Process logcatProc = null;
                BufferedReader mReader = null;
                try
                {
                    Log.i(TAG, Thread.currentThread().getName() + ": Thread started");
                    final String[] cmd = {"/bin/sh", "-c", m_command};
                    logcatProc = Runtime.getRuntime().exec(cmd);
                    mReader = new BufferedReader(new InputStreamReader(logcatProc.getInputStream(), StandardCharsets.UTF_8), 1024);
                    String line;
                    while (m_running)
                    {
                        while(m_paused)
                        {
                            Log.i(TAG, Thread.currentThread().getName() + ": Thread waiting");
                            m_locker.wait();
                        }

                        line = mReader.readLine();
                        if(null == line || line.isEmpty())
                        {
                            sleep(WAIT_MS);
                            sleepCount++;
                            continue;
                        }
                        if(null != m_callback)
                            m_callback.Output(line);
                    }
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }
                finally
                {
                    m_running = false;
                    if (null != logcatProc)
                    {
                        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                        {
                            if (logcatProc.isAlive())
                                logcatProc.destroy();
                        }
                        else
                            logcatProc.destroy();
                    }
                    Q3EUtils.Close(mReader);
                    Log.i(TAG, Thread.currentThread().getName() + ": Thread quit");
                }
            }
        }
    }
}
