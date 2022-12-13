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
import android.annotation.SuppressLint;
import android.graphics.Typeface;

public class Q3EMain extends Activity {
	public static Q3ECallbackObj mAudio;
	public static Q3EView mGLSurfaceView;
    public static String datadir;	
    // k
    private boolean m_hideNav = true;
    private int m_runBackground = 1;
    private int m_renderMemStatus = 0;
    @SuppressLint("InlinedApi")
    private final int m_uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
    | View.SYSTEM_UI_FLAG_IMMERSIVE
    | View.SYSTEM_UI_FLAG_FULLSCREEN;
    public static Q3EControlView mControlGLSurfaceView;
    private MemInfoTextView memoryUsageText;
	
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
	
    @SuppressLint("ResourceType")
	@Override
	public void onCreate(Bundle savedInstanceState) {
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		
		if (Build.VERSION.SDK_INT>=9)
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
            
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        m_hideNav = preferences.getBoolean("harm_hide_nav", true);
        m_renderMemStatus = preferences.getInt("harm_render_mem_status", 0);
        m_runBackground = Integer.parseInt(preferences.getString("harm_run_background", "1"));
        //k
        if(m_hideNav)
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
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
        }
        Q3EUtils.q3ei.VOLUME_UP_KEY_CODE = preferences.getInt("harm_volume_up_key", Q3EKeyCodes.KeyCodes.K_F3);
        Q3EUtils.q3ei.VOLUME_DOWN_KEY_CODE = preferences.getInt("harm_volume_down_key", Q3EKeyCodes.KeyCodes.K_F2);
        Q3EUtils.q3ei.SetupEngineLib(); //k setup engine library here again
        Q3EUtils.q3ei.view_motion_control_gyro = preferences.getBoolean(Q3EUtils.pref_harm_view_motion_control_gyro, false);
		
		super.onCreate(savedInstanceState);
		
		if (Q3EUtils.q3ei==null)
		{			
			finish();
			try
			{
				//k startActivity(new Intent(this,Class.forName(getPackageName()+".GameLauncher")));//Dirty hack	
				startActivity(new Intent(this,Class.forName("com.n0n3m4.DIII4A.GameLauncher")));//Dirty hack
			}			
			catch (Exception e){e.printStackTrace();};			
			return;
		}
		
		datadir=preferences.getString(Q3EUtils.pref_datapath, Q3EUtils.q3ei.default_path);
		if ((datadir.length()>0)&&(datadir.charAt(0)!='/'))//lolwtfisuserdoing?
		{
			datadir="/"+datadir;
			preferences.edit().putString(Q3EUtils.pref_datapath, datadir).commit();
		}
		if(checkGameFiles())
		{
            Q3EJNI.SetRedirectOutputToFile(preferences.getBoolean("harm_redirect_output_to_file", true));
            Q3EJNI.SetNoHandleSignals(preferences.getBoolean("harm_no_handle_signals", false));
			if (mGLSurfaceView==null)
                mGLSurfaceView = new Q3EView(this);
			if (mControlGLSurfaceView==null)
               mControlGLSurfaceView = new Q3EControlView(this);
            mControlGLSurfaceView.EnableGyroscopeControl(Q3EUtils.q3ei.view_motion_control_gyro);
            float gyroXSens = preferences.getFloat(Q3EUtils.pref_harm_view_motion_gyro_x_axis_sens, Q3EControlView.GYROSCOPE_X_AXIS_SENS); 
            float gyroYSens = preferences.getFloat(Q3EUtils.pref_harm_view_motion_gyro_y_axis_sens, Q3EControlView.GYROSCOPE_Y_AXIS_SENS);
            if(Q3EUtils.q3ei.view_motion_control_gyro && (gyroXSens != 0.0f || gyroYSens != 0.0f))
                mControlGLSurfaceView.SetGyroscopeSens(gyroXSens, gyroYSens);
            mControlGLSurfaceView.RenderView(mGLSurfaceView);
            RelativeLayout mainLayout = new RelativeLayout(this);
            ViewGroup.LayoutParams params = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mainLayout.addView(mGLSurfaceView, params);
            mainLayout.addView(mControlGLSurfaceView, params);
            if(m_renderMemStatus > 0) //k
            {
                memoryUsageText = new MemInfoTextView(mainLayout.getContext());
                params = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                mainLayout.addView(memoryUsageText, params);
                memoryUsageText.setTypeface(Typeface.MONOSPACE);
            }
            setContentView(mainLayout);

            mControlGLSurfaceView.requestFocus();
			if (mAudio==null)			
			{
			mAudio = new Q3ECallbackObj();
			mAudio.vw=mControlGLSurfaceView;
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
        if(null != mGLSurfaceView)
		    Q3EJNI.shutdown();    
		super.onDestroy();		
	}

	@Override
	protected void onPause() {
		super.onPause();
        
        //k
        if(memoryUsageText != null)
            memoryUsageText.Stop();

        if(m_runBackground < 2)
		if(mAudio != null)
		{
			mAudio.pause();			
		}

        if(m_runBackground < 1)
        {
            if(mGLSurfaceView != null)
            {
                mGLSurfaceView.onPause();
            }   

            if(mControlGLSurfaceView != null)
            {
                mControlGLSurfaceView.onPause();
            }		   
        }
	}

	@Override
	protected void onResume() {
		super.onResume();						

        //k
        if(memoryUsageText != null/* && m_renderMemStatus > 0*/)
            memoryUsageText.Start(m_renderMemStatus * 1000);

        //k if(m_runBackground < 1)
		if(mGLSurfaceView != null)
		{
			mGLSurfaceView.onResume();
		}			
        if(mControlGLSurfaceView != null)
        {
            mControlGLSurfaceView.onResume();
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
    
    private class MemInfoTextView extends TextView
    {
        private MemDumpFunc m_memFunc = null;
        
        public MemInfoTextView(Context context)
        {
            super(context);
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

                Debug.MemoryInfo memInfos[] = m_am.getProcessMemoryInfo(m_processs);
                Debug.MemoryInfo memInfo = memInfos[0];
                if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) // 23
                {
                    java_mem = Integer.valueOf(memInfo.getMemoryStat("summary.java-heap")) / UNIT;
                    native_mem = Integer.valueOf(memInfo.getMemoryStat("summary.native-heap")) / UNIT;
                    graphics_mem = Integer.valueOf(memInfo.getMemoryStat("summary.graphics")) / UNIT;   
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

                StringBuilder sb = new StringBuilder();
                sb.append("App->");
                sb.append("Dalvik:").append(java_mem).append("|");
                sb.append("Native:").append(native_mem).append("|");
                sb.append("Graphics:").append(graphics_mem_str).append("|");
                sb.append("≈").append(total_used_str).append("\n");

                sb.append("Sys->");
                sb.append("Used:").append(usedMem);
                sb.append("/Total:").append(totalMem).append("|");
                sb.append(percent + "%").append("|");
                sb.append("≈").append(availMem);

                /*
                 final int totalMem32 = 4 * 1024;
                 final boolean needShow32 = !Q3EJNI.IS_64 || totalMem > totalMem32;
                 if(needShow32)
                 {
                 percent = Math.round(Math.min(1.0f, ((float)usedMem / (float)totalMem32)) * 100);
                 availMem = totalMem32 - usedMem;
                 sb.append("\n  32:");
                 sb.append("Used:").append(usedMem);
                 sb.append("/Total:").append(totalMem32).append("|");
                 sb.append(percent + "%").append("|");
                 sb.append("≈").append(availMem);
                 sb.append("(+").append(totalMem - totalMem32).append(")");
                 }
                 */

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
    
}
