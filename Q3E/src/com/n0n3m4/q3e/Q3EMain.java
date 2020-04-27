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

public class Q3EMain extends Activity {
	public static Q3ECallbackObj mAudio;
	public static Q3EView mGLSurfaceView;
    public static String datadir;	
	
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
		
		if(mAudio != null)
		{
			mAudio.pause();			
		}
		
		if(mGLSurfaceView != null)
		{
			mGLSurfaceView.onPause();
		}		
	}

	@Override
	protected void onResume() {
		super.onResume();						
		
		if(mGLSurfaceView != null)
		{
			mGLSurfaceView.onResume();
		}			
		
		if(mAudio != null)
		{			
			mAudio.resume();
		}		
	}
}
