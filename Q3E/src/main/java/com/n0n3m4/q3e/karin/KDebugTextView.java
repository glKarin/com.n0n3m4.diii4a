package com.n0n3m4.q3e.karin;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Debug;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.widget.TextView;

import java.util.Timer;
import java.util.TimerTask;

public class KDebugTextView extends TextView {
    private MemDumpFunc m_memFunc = null;

    @SuppressLint("ResourceType")
    public KDebugTextView(Context context)
    {
        super(context);
        setFocusable(false);
        setFocusableInTouchMode(false);
        setTextColor(Color.WHITE);
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // 23
            setTextAppearance(android.R.attr.textAppearanceMedium);
        setPadding(10, 5, 10, 5);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            setAlpha(0.75f);
        }
        setTypeface(Typeface.MONOSPACE);
        m_memFunc = new
                MemDumpFunc_timer
                //MemDumpFunc_handler
                (this);
    }

    public void Start(int interval)
    {
        if(m_memFunc != null && interval > 0)
            m_memFunc.Start(interval);
    }

    public void Stop()
    {
        if(m_memFunc != null)
            m_memFunc.Stop();
    }

    private abstract class MemDumpFunc
    {
        private static final int UNIT = 1024;
        private static final int UNIT2 = 1024 * 1024;

        private boolean  m_lock = false;
        private ActivityManager m_am = null;
        private final int[] m_processs = {Process.myPid()};
        private final ActivityManager.MemoryInfo m_outInfo = new ActivityManager.MemoryInfo();
        private TextView m_memoryUsageText;
        protected Runnable m_runnable = new Runnable() {
            @Override
            public void run()
            {
                if (IsLock())
                    return;
                Lock();
                final String text = GetMemText();
                HandleMemText(text);
            }
        };

        public MemDumpFunc(TextView view)
        {
            m_memoryUsageText = view;
        }

        public void Start(int interval)
        {
            Stop();
            m_am = (ActivityManager)getContext().getSystemService(Context.ACTIVITY_SERVICE);
            Unlock();
        }

        public void Stop()
        {
            Unlock();
        }

        private void Lock()
        {
            m_lock = true;
        }

        private void Unlock()
        {
            m_lock = false;
        }

        private boolean IsLock()
        {
            return m_lock;
        }

        private String GetMemText()
        {
            m_am.getMemoryInfo(m_outInfo);
            int availMem = -1;
            int totalMem = -1;
            int usedMem = -1;
            int java_mem = -1;
            int native_mem = -1;
            int graphics_mem = -1;

            availMem = (int)(m_outInfo.availMem / UNIT2);
            if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) // 16
            {
                totalMem = (int)(m_outInfo.totalMem / UNIT2);
                usedMem = (int)((m_outInfo.totalMem - m_outInfo.availMem) / UNIT2);
            }

            Debug.MemoryInfo[] memInfos = m_am.getProcessMemoryInfo(m_processs);
            Debug.MemoryInfo memInfo = memInfos[0];
            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.P) {
                Debug.MemoryInfo memoryInfo = new Debug.MemoryInfo();
                Debug.getMemoryInfo(memoryInfo);
                java_mem = Integer.parseInt(memoryInfo.getMemoryStat("summary.java-heap")) / UNIT;
                native_mem = Integer.parseInt(memoryInfo.getMemoryStat("summary.native-heap")) / UNIT;
                //String code = memoryInfo.getMemoryStat("summary.code");
                //String stack = memoryInfo.getMemoryStat("summary.stack");
                graphics_mem = Integer.parseInt(memoryInfo.getMemoryStat("summary.graphics")) / UNIT;
                //String privateOther = memoryInfo.getMemoryStat("summary.private-other");
                //String system = memoryInfo.getMemoryStat("summary.system");
                //String swap = memoryInfo.getMemoryStat("summary.total-swap");
            }
            else if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // 23 // > Android P, slow frequency
            {
                java_mem = Integer.parseInt(memInfo.getMemoryStat("summary.java-heap")) / UNIT;
                native_mem = Integer.parseInt(memInfo.getMemoryStat("summary.native-heap")) / UNIT;
                graphics_mem = Integer.parseInt(memInfo.getMemoryStat("summary.graphics")) / UNIT;
                //String stack_mem = memInfo.getMemoryStat("summary.stack");
                //String code_mem = memInfo.getMemoryStat("summary.code");
                //String others_mem = memInfo.getMemoryStat("summary.system");
            }
            else
            {
                java_mem = memInfo.dalvikPrivateDirty / UNIT;
                native_mem = memInfo.nativePrivateDirty / UNIT;
            }

            int total_used = native_mem + java_mem;
            String total_used_str = graphics_mem >= 0 ? "" + (total_used + graphics_mem) : (total_used + "<Excluding graphics memory>");
            String graphics_mem_str = graphics_mem >= 0 ? "" + graphics_mem : "unknown";
            int percent = Math.round(((float)usedMem / (float)totalMem) * 100);
            availMem = totalMem - usedMem;

            String sb = "App->" +
                    "Dalvik:" + java_mem + "|" +
                    "Native:" + native_mem + "|" +
                    "Graphics:" + graphics_mem_str + "|" +
                    "≈" + total_used_str + "\n" +
                    "Sys->" +
                    "Used:" + usedMem +
                    "/Total:" + totalMem + "|" +
                    percent + "%" + "|" +
                    "≈" + availMem;
            return sb;
        }

        private void HandleMemText(final String text)
        {
            m_memoryUsageText.post(new Runnable(){
                public void run()
                {
                    m_memoryUsageText.setText(text);
                    Unlock();
                }
            });
        }
    }

    private class MemDumpFunc_timer extends MemDumpFunc
    {
        private Timer m_timer = null;

        public MemDumpFunc_timer(TextView view)
        {
            super(view);
        }

        @Override
        public void Start(int interval)
        {
            super.Start(interval);
            TimerTask task = new TimerTask(){
                @Override
                public void run()
                {
                    m_runnable.run();
                }
            };

            m_timer = new Timer();
            m_timer.scheduleAtFixedRate(task, 0, interval);
        }

        @Override
        public void Stop()
        {
            super.Stop();
            if(m_timer != null)
            {
                m_timer.cancel();
                m_timer.purge();
                m_timer = null;
            }
        }
    }

    private class MemDumpFunc_handler extends MemDumpFunc
    {
        private HandlerThread m_thread = null;
        private Handler m_handler = null;
        private Runnable m_handlerCallback = null;

        public MemDumpFunc_handler(TextView view)
        {
            super(view);
        }

        @Override
        public void Start(final int interval)
        {
            super.Start(interval);
            m_thread = new HandlerThread("MemDumpFunc_thread");
            m_thread.start();
            m_handler = new Handler(m_thread.getLooper());
            m_handlerCallback = new Runnable(){
                public void run()
                {
                    m_runnable.run();
                    m_handler.postDelayed(m_handlerCallback, interval);
                }
            };
            m_handler.post(m_handlerCallback);
        }

        @Override
        public void Stop()
        {
            super.Stop();
            if(m_handler != null)
            {
                if(m_handlerCallback != null)
                {
                    m_handler.removeCallbacks(m_handlerCallback);
                    m_handlerCallback = null;
                }
                m_handler = null;
            }
            if(m_thread != null)
            {
                m_thread.quit();
                m_thread = null;
            }
        }
    }
}
