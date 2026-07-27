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
import android.view.MotionEvent;
import android.view.View;
import android.widget.TextView;

import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.R;

import java.util.Timer;
import java.util.TimerTask;

public class KDebugTextView extends TextView {
    private MemDumpFunc m_memFunc = null;
    private int         m_lastX;
    private int         m_lastY;
    private boolean     m_pressed = false;

    public KDebugTextView(Context context)
    {
        super(context);
        Setup();
    }

    @SuppressLint("ResourceType")
    private void Setup()
    {
        setFocusable(false);
        setFocusableInTouchMode(false);
        setTextColor(Color.WHITE);
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // 23
            setTextAppearance(android.R.attr.textAppearanceSmall);
        //else
            setTextSize(10);
        setPadding(10, 5, 10, 5);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            setAlpha(0.75f);
        }
        setTypeface(Typeface.MONOSPACE);
        m_memFunc = new
                MemDumpFunc_timer
                //MemDumpFunc_handler
                (this);
        setOnTouchListener(m_onTouchEvent);
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

    public void ShowBackground()
    {
        //setBackgroundColor(Color.argb(128, 0, 0, 0));
        setBackgroundResource(R.drawable.debug_text_background);
    }

    private final View.OnTouchListener m_onTouchEvent = new OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent ev) {
            int x = (int)ev.getRawX();
            int y = (int)ev.getRawY();
            //Log.e("TAGID_TAG", String.format("%d %d|%d", x, y, ev.getAction()));
            switch (ev.getAction())
            {
                case MotionEvent.ACTION_DOWN:
                    if(!m_pressed)
                    {
                        m_pressed = true;
                        m_lastX = x;
                        m_lastY = y;
                        return true;
                    }
                    break;
                case MotionEvent.ACTION_MOVE:
                    if(m_pressed)
                    {
                        int lastDeltaX = x - m_lastX;
                        int lastDeltaY = y - m_lastY;
                        boolean update = false;
                        if(lastDeltaX != 0)
                        {
                            int curx = (int) getX();
                            int posx = curx + lastDeltaX;
                            if(posx < 0)
                                posx = 0;
                            if(curx != posx)
                            {
                                setX(posx);
                                Q3EPreference.SetStringFromInt(getContext(), Q3EPreference.pref_harm_debug_text_x, posx);
                                update = true;
                            }
                        }
                        if(lastDeltaY != 0)
                        {
                            int cury = (int) getY();
                            int posy = cury + lastDeltaY;
                            if(posy < 0)
                                posy = 0;
                            if(cury != posy)
                            {
                                setY(posy);
                                Q3EPreference.SetStringFromInt(getContext(), Q3EPreference.pref_harm_debug_text_y, posy);
                                update = true;
                            }
                        }
                        if(update)
                            getParent().requestLayout();
                        m_lastX = x;
                        m_lastY = y;
                        return true;
                    }
                    break;
                case MotionEvent.ACTION_UP:
                    if(m_pressed)
                    {
                        ResetTouch();
                        return true;
                    }
                    break;
                case MotionEvent.ACTION_CANCEL:
                    ResetTouch();
                    return true;
                default:
                    break;
            }
            return false;
        }
    };

    private void ResetTouch()
    {
        m_lastX = 0;
        m_lastY = 0;
        m_pressed = false;
    }

    private abstract class MemDumpFunc
    {
        private boolean  m_lock = false;
        private ActivityManager m_am = null;
        private final int[] m_processs = {Process.myPid()};
        private final ActivityManager.MemoryInfo m_outInfo = new ActivityManager.MemoryInfo();
        private final KMemoryInfo m_memoryInfo = new KMemoryInfo();
        private final TextView m_memoryUsageText;
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
            m_memoryInfo.Invalid();
            m_memoryInfo.Get(m_am, m_processs, m_outInfo);
            m_memoryInfo.Mb();

            long app_total_used = m_memoryInfo.native_memory + m_memoryInfo.java_memory;
            String total_used_str = m_memoryInfo.graphics_memory >= 0 ? "" + (app_total_used + m_memoryInfo.graphics_memory) : (app_total_used + "(Excluding graphics memory)");
            String graphics_mem_str = m_memoryInfo.graphics_memory >= 0 ? "" + m_memoryInfo.graphics_memory : "<unknown>";
            int percent = (int)Math.round(((double) m_memoryInfo.used_memory / (double)m_memoryInfo.total_memory) * 100);
            long availMem = m_memoryInfo.total_memory - m_memoryInfo.used_memory;

            String sb = "App:"
                    + "Dalvik(" + m_memoryInfo.java_memory + ")+"
                    + "Native(" + m_memoryInfo.native_memory + ")+"
                    + "Graphics(" + graphics_mem_str + ")"
                    + "≈" + total_used_str + "\n"
                    + "Sys:"
                    + "Used(" + m_memoryInfo.used_memory + ")/"
                    + "Total(" + m_memoryInfo.total_memory + ")"
                    + "≈" + percent + "%"
                    + "-=" + availMem
                    ;
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
