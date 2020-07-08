/*
	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import java.io.File;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;
import android.widget.RelativeLayout;
import android.view.ViewGroup;
import android.widget.TextView;
import java.util.Timer;
import java.util.TimerTask;
import android.graphics.Color;
import android.app.ActivityManager;
import android.os.Process;
import android.os.Debug;
import android.view.View;
import android.os.HandlerThread;
import android.os.Handler;
import android.content.SharedPreferences;

public class Q3EMain extends Activity {
	public static Q3ECallbackObj mAudio;
	public static Q3EView mGLSurfaceView;
    public static String datadir;	
    // k
    private MemDumpFunc m_memFunc = null;
    private boolean m_hideNav = false;
    private int m_runBackground = 0;
    private final int m_uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
    | View.SYSTEM_UI_FLAG_IMMERSIVE
    | View.SYSTEM_UI_FLAG_FULLSCREEN;
	
	public void ShowMessage(String s)
	{
		Toast.makeText(this, s, Toast.LENGTH_LONG).show();		
	}

	public boolean checkGameFiles()
	{		
		if(!new File(datadir).exists())
		{
			ShowMessage("Game files weren't found: put game files to "+datadir);
			this.finish();
			return false;
		}
		
		return true;
	}	
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		
		if (Build.VERSION.SDK_INT>=9)
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
            
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        m_hideNav = preferences.getBoolean(Q3EUtils.pref_harm_hide_nav, false);
        m_runBackground = preferences.getInt(Q3EUtils.pref_harm_run_background, 0);
        //k
        if(m_hideNav)
        {
            final View decorView = getWindow().getDecorView();
            decorView.setSystemUiVisibility(m_uiOptions);// This code will always hide the navigation bar
/*            decorView.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener(){
                    @Override
                    public void  onSystemUiVisibilityChange(int visibility)
                    {
                        if((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0)
                        {
                            decorView.setSystemUiVisibility(m_uiOptions);
                        }
                    }
                });*/
        }
		
		super.onCreate(savedInstanceState);
		
		if (Q3EUtils.q3ei==null)
		{			
			finish();
			try
			{
				startActivity(new Intent(this,Class.forName(getPackageName()+".GameLauncher")));//Dirty hack	
			}			
			catch (Exception e){e.printStackTrace();};			
			return;
		}
		
		datadir=PreferenceManager.getDefaultSharedPreferences(this).getString(Q3EUtils.pref_datapath, Q3EUtils.q3ei.default_path);
		if ((datadir.length()>0)&&(datadir.charAt(0)!='/'))//lolwtfisuserdoing?
		{
			datadir="/"+datadir;
			PreferenceManager.getDefaultSharedPreferences(this).edit().putString(Q3EUtils.pref_datapath, datadir).commit();
		}
		if(checkGameFiles())
		{
			if (mGLSurfaceView==null)
			mGLSurfaceView = new Q3EView(this);			
            if(PreferenceManager.getDefaultSharedPreferences(this).getBoolean(Q3EUtils.pref_harm_render_mem_status, false)) //k
            {
                RelativeLayout mainLayout = new RelativeLayout(this);
                ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
                mainLayout.addView(mGLSurfaceView, params);
                final TextView memoryUsageText = new TextView(mainLayout.getContext());
                memoryUsageText.setTextColor(Color.WHITE);
                memoryUsageText.setTextAppearance(android.R.attr.textAppearanceMedium);
                memoryUsageText.setPadding(10, 5, 10, 5);
                memoryUsageText.setAlpha(0.8f);
                params = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                mainLayout.addView(memoryUsageText, params);
                setContentView(mainLayout);   
                
                m_memFunc = new 
                MemDumpFunc_timer
                //MemDumpFunc_handler
                (memoryUsageText);
            }
            else
                setContentView(mGLSurfaceView);
                
			mGLSurfaceView.requestFocus();			
			if (mAudio==null)			
			{
			mAudio = new Q3ECallbackObj();
			mAudio.vw=mGLSurfaceView;
			}
			Q3EJNI.setCallbackObject(mAudio);
		}
		else
		{
			finish();
		}
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();		
	}

	@Override
	protected void onPause() {
		super.onPause();
        
        //k
        if(m_memFunc != null)
            m_memFunc.Stop();

        if(m_runBackground < 2)
		if(mAudio != null)
		{
			mAudio.pause();			
		}

        if(m_runBackground < 1)
		if(mGLSurfaceView != null)
		{
			mGLSurfaceView.onPause();
		}		
	}

	@Override
	protected void onResume() {
		super.onResume();						

        //k
        if(m_memFunc != null)
            m_memFunc.Start(5000);

        //k if(m_runBackground < 1)
		if(mGLSurfaceView != null)
		{
			mGLSurfaceView.onResume();
		}			

        //k if(m_runBackground < 2)
		if(mAudio != null)
		{			
			mAudio.resume();
		}
	}

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        if(m_hideNav)
        {
            getWindow().getDecorView().setSystemUiVisibility(m_uiOptions);
        }
    }
    
    private abstract class MemDumpFunc
    {
        private static final int UNIT = 1024;
        private static final int UNIT2 = 1024 * 1024;
        
        private boolean  m_lock = false;
        private ActivityManager m_am = null;
        private int m_processs[] = {Process.myPid()};
        private ActivityManager.MemoryInfo m_outInfo = new ActivityManager.MemoryInfo();
        private TextView m_memoryUsageText = null;
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
            m_am = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
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
            int availMem = (int)(m_outInfo.availMem / UNIT2);
            int totalMem = (int)(m_outInfo.totalMem / UNIT2);
            int usedMem = (int)((m_outInfo.totalMem - m_outInfo.availMem) / UNIT2);

            Debug.MemoryInfo memInfos[] = m_am.getProcessMemoryInfo(m_processs);
            Debug.MemoryInfo memInfo = memInfos[0];
            int java_mem = Integer.valueOf(memInfo.getMemoryStat("summary.java-heap")) / UNIT;
            int native_mem = Integer.valueOf(memInfo.getMemoryStat("summary.native-heap")) / UNIT;
            int graphics_mem = Integer.valueOf(memInfo.getMemoryStat("summary.graphics")) / UNIT;

            //String stack_mem = memInfo.getMemoryStat("summary.stack");
            //String code_mem = memInfo.getMemoryStat("summary.code");
            //String others_mem = memInfo.getMemoryStat("summary.system");
            StringBuffer sb = new StringBuffer();
            //sb.append("Dalvik heap(").append(java_mem).append(") ");
            sb.append("Native[+Dalvik] heap(").append(native_mem).append("[+").append(java_mem).append("]) ");
            sb.append("Graphics(").append(graphics_mem).append(")");
            sb.append(" [≈").append(graphics_mem + native_mem + java_mem).append("]\n");
            sb.append("Usage(").append(usedMem).append('/').append(totalMem).append('=').append(availMem)
            .append("[-").append(graphics_mem).append('≈').append(availMem - graphics_mem).append(']')
            .append(")");


            return sb.toString();
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
