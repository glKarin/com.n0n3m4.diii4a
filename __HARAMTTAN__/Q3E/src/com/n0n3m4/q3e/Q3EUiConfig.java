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

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.View;
import android.util.Log;

public class Q3EUiConfig extends Activity {
	
	Q3EUiView vw;
    //k
    private boolean m_hideNav = false;
    private int m_uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
    | View.SYSTEM_UI_FLAG_IMMERSIVE
    | View.SYSTEM_UI_FLAG_FULLSCREEN;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);    	
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		
		if (Build.VERSION.SDK_INT>=9)
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

        //k
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        m_hideNav = preferences.getBoolean(Q3EUtils.pref_harm_hide_nav, false);
        if(m_hideNav)
        {
            final View decorView = getWindow().getDecorView();
            decorView.setSystemUiVisibility(m_uiOptions);
        }
		
		super.onCreate(savedInstanceState);
		vw=new Q3EUiView(this);
		setContentView(vw);
	}
	
	@Override
	protected void onPause() {
		vw.SaveAll();
		super.onPause();
	}
	
	@Override
	public void onBackPressed() {
		vw.SaveAll();
		super.onBackPressed();
	}
    
    //k
    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        if(m_hideNav)
        {
            getWindow().getDecorView().setSystemUiVisibility(m_uiOptions);
        }
    }
}
