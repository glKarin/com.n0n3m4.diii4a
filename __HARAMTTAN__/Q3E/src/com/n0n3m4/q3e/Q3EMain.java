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

public class Q3EMain extends Activity {
	public static Q3ECallbackObj mAudio;
	public static Q3EView mGLSurfaceView;
    public static String datadir;	
    // k
    private Timer m_timer = null;
    private boolean  m_lock = false;
	
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
                memoryUsageText.setTextSize(20);
                memoryUsageText.setPadding(10, 5, 10, 5);
                params = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                mainLayout.addView(memoryUsageText, params);
                setContentView(mainLayout);   

                final ActivityManager am = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
                final int processs[] = {Process.myPid()};
                final int unit = 1024;
                final int unit2 = 1024 * 1024;
                final ActivityManager.MemoryInfo outInfo = new ActivityManager.MemoryInfo();
                TimerTask task = new TimerTask(){
                    @Override
                    public void run()
                    {
                        if (m_lock)
                            return;
                        m_lock = true;
                        am.getMemoryInfo(outInfo);
                        int availMem = (int)(outInfo.availMem / unit2);
                        int totalMem = (int)(outInfo.totalMem / unit2);
                        int usedMem = (int)((outInfo.totalMem - outInfo.availMem) / unit2);

                        Debug.MemoryInfo memInfos[] = am.getProcessMemoryInfo(processs);
                        Debug.MemoryInfo memInfo = memInfos[0];
                        //int java_mem = Integer.valueOf(memInfo.getMemoryStat("summary.java-heap")) / unit;
                        int native_mem = Integer.valueOf(memInfo.getMemoryStat("summary.native-heap")) / unit;
                        int graphics_mem = Integer.valueOf(memInfo.getMemoryStat("summary.graphics")) / unit;

                        //String stack_mem = memInfo.getMemoryStat("summary.stack");
                        //String code_mem = memInfo.getMemoryStat("summary.code");
                        //String others_mem = memInfo.getMemoryStat("summary.system");
                        StringBuffer sb = new StringBuffer();
                        //sb.append("Dalvik heap(").append(java_mem).append(") ");
                        sb.append("Native heap(").append(native_mem).append(") ");
                        sb.append("Graphics(").append(graphics_mem).append(")\n");
                        sb.append("Usage(").append(usedMem).append("/").append(totalMem).append("=").append(availMem).append(")");


                        final String text = sb.toString();
                        runOnUiThread(new Runnable(){
                                public void run()
                                {        
                                    memoryUsageText.setText(text);
                                    m_lock = false;
                                }
                            });
                    }
                };
                
                m_timer = new Timer();
                m_timer.scheduleAtFixedRate(task, 0, 5000);
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

        if(m_timer != null)
            m_timer.cancel();
	}

	@Override
	protected void onPause() {
		super.onPause();

        int harmRunBackground = PreferenceManager.getDefaultSharedPreferences(this).getInt(Q3EUtils.pref_harm_run_background, 0);
        if(harmRunBackground < 2)
		if(mAudio != null)
		{
			mAudio.pause();			
		}

        if(harmRunBackground < 1)
		if(mGLSurfaceView != null)
		{
			mGLSurfaceView.onPause();
		}		
	}

	@Override
	protected void onResume() {
		super.onResume();						

        int harmRunBackground = PreferenceManager.getDefaultSharedPreferences(this).getInt(Q3EUtils.pref_harm_run_background, 0);
        if(harmRunBackground < 1)
		if(mGLSurfaceView != null)
		{
			mGLSurfaceView.onResume();
		}			

        if(harmRunBackground < 2)
		if(mAudio != null)
		{			
			mAudio.resume();
		}
	}
}
