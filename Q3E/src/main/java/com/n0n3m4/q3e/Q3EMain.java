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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.KeyEvent;
import android.view.PointerIcon;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.n0n3m4.q3e.device.Q3EOuya;
import com.n0n3m4.q3e.gl.Q3EGL;
import com.n0n3m4.q3e.karin.KDebugTextView;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KMouseCursor;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KUncaughtExceptionHandler;
import com.n0n3m4.q3e.karin.KidTechCommand;

public class Q3EMain extends Activity
{
    private       Q3ECallbackObj mAudio;
    private       Q3EView        mGLSurfaceView;
    private       RelativeLayout mainLayout;
    private       KMouseCursor   mouseCursor;
    // k
    private       boolean        m_hideNav         = true;
    private       int            m_runBackground   = 1;
    private       int            m_renderMemStatus = 0;
    private       Q3EControlView mControlGLSurfaceView;
    private       KDebugTextView memoryUsageText;
    private       boolean        m_coverEdges      = true;
    private       boolean        m_portrait        = false;
    private       int            m_offsetY         = 0;
    @SuppressLint("StaticFieldLeak")
    public static Q3EGameHelper  gameHelper;

    private       Q3EKeyboard keyboard;
    private static final int VIEW_BASE_Z = 100;

    /*
    Intent::extras
    game: game type
    command: command line arguments
     */
    @SuppressLint("ResourceType")
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        Q3E.activity = this;

        keyboard = new Q3EKeyboard(this);

        gameHelper = new Q3EGameHelper();
        gameHelper.SetContext(this);

        // setup fullscreen
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        // make screen always on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // setup fullscreen
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        // exception handler
        KUncaughtExceptionHandler.HandleUnexpectedException(this);

        // init game environment
        SetupGame();

        // init gui props
        InitProps();

        // setup screen edges
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && m_coverEdges)
        {
            WindowManager.LayoutParams lp = getWindow().getAttributes();
            lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(lp);
        }

        // force landscape orientation
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) // 9
        {
            if(m_portrait)
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
            else
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        }

        // hide navigation bar
        SetupUIFlags();

        // create
        super.onCreate(savedInstanceState);

        // check start
        if(!CheckStart())
            return;

        Q3EUtils.DumpPID(this);

        // setup language environment
        Q3ELang.Locale(this);

        // load game
        if(gameHelper.checkGameFiles())
        {
            // extract game required resource in apk
            gameHelper.ExtractGameResource();

            // init GUI component
            InitGUI();
        }
        else
        {
            finish();
            Q3EUtils.RunLauncher(this);
        }
    }

    @Override
    public void onAttachedToWindow()
    {
        super.onAttachedToWindow();

        keyboard.onAttachedToWindow();
    }

    @Override
    protected void onDestroy()
    {
        KLog.I("Q3EMain::onDestroy -> %b", Q3E.running);
        Q3E.activity = null;
        Q3E.gameView = null;
        Q3E.controlView = null;
        Q3E.Stop();

/*        if (null != mGLSurfaceView)
            mGLSurfaceView.Shutdown();*/

        super.onDestroy();
        if(null != mAudio)
            mAudio.OnDestroy();
    }

    @Override
    protected void onPause()
    {
        super.onPause();

        //k
        if(memoryUsageText != null)
            memoryUsageText.Stop();

        if(m_runBackground < 2)
            if(mAudio != null)
            {
                mAudio.pause();
            }

        Q3E.Pause();

        if(mControlGLSurfaceView != null)
        {
            mControlGLSurfaceView.Pause();
        }
        if(m_initView)
            Q3EUtils.CloseVKB(mGLSurfaceView);
        keyboard.OnPause();
    }

    @Override
    protected void onResume()
    {
        super.onResume();

        //k
        if(memoryUsageText != null/* && m_renderMemStatus > 0*/)
            memoryUsageText.Start(m_renderMemStatus * 1000);

        //k if(m_runBackground < 1)
        Q3E.Resume();
        if(mControlGLSurfaceView != null)
        {
            mControlGLSurfaceView.Resume();
        }

        //k if(m_runBackground < 2)
        if(mAudio != null)
        {
            mAudio.resume();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        super.onConfigurationChanged(newConfig);
        if(newConfig.orientation == Configuration.ORIENTATION_PORTRAIT && m_portrait)
        {
            InitView();
            //keyboard.onAttachedToWindow(m_offsetY);
        }
/*        else if(newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE && !m_portrait)
        {
            keyboard.onAttachedToWindow();
        }*/
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        SetupUIFlags();
    }

    private void SetupUIFlags()
    {
        final View decorView = getWindow().getDecorView();
        if(m_hideNav)
            decorView.setSystemUiVisibility(Q3EUtils.UI_FULLSCREEN_HIDE_NAV_OPTIONS);
        else
            decorView.setSystemUiVisibility(Q3EUtils.UI_FULLSCREEN_OPTIONS);
/*        if(m_hideNav)
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
				final View decorView = getWindow().getDecorView();
				decorView.setSystemUiVisibility(m_uiOptions);// This code will always hide the navigation bar
            decorView.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener(){
                    @Override
                    public void  onSystemUiVisibilityChange(int visibility)
                    {
                        if((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0)
                        {
                            decorView.setSystemUiVisibility(m_uiOptions);
                        }
                    }
                });
			}
        }*/
    }

    private void InitProps()
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        Q3EGL.usegles20 = false;
        m_coverEdges = preferences.getBoolean(Q3EPreference.COVER_EDGES, true);

        m_hideNav = preferences.getBoolean(Q3EPreference.HIDE_NAVIGATION_BAR, true);
        m_renderMemStatus = preferences.getInt(Q3EPreference.RENDER_MEM_STATUS, 0);
        m_portrait = preferences.getBoolean(Q3EPreference.pref_harm_portrait, false);
        String harm_run_background = preferences.getString(Q3EPreference.RUN_BACKGROUND, "1");
        if(null != harm_run_background)
            m_runBackground = Integer.parseInt(harm_run_background);
        else
            m_runBackground = 1;
    }

    private void InitGUI()
    {
        if(!Q3EOuya.Init(this))
            Q3EUtils.isOuya = false;

        if(mAudio == null)
            mAudio = new Q3ECallbackObj();
        mAudio.InitGUIInterface(this);
        Q3EUtils.q3ei.callbackObj = mAudio;
        Q3EJNI.setCallbackObject(mAudio);

        mainLayout = new RelativeLayout(this);

        if(!m_portrait)
            InitView();

        setContentView(mainLayout);
    }

    private boolean m_initView = false;

    private void InitView()
    {
        if(m_initView)
            return;

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);

        if(mGLSurfaceView == null)
            mGLSurfaceView = new Q3EView(this);
        Q3E.gameView = mGLSurfaceView;
        if(mControlGLSurfaceView == null)
            mControlGLSurfaceView = new Q3EControlView(this);
        Q3E.controlView = mControlGLSurfaceView;
        mAudio.vw = mControlGLSurfaceView;
        mControlGLSurfaceView.EnableGyroscopeControl(Q3EUtils.q3ei.view_motion_control_gyro);
        float gyroXSens = preferences.getFloat(Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EControlView.GYROSCOPE_X_AXIS_SENS);
        float gyroYSens = preferences.getFloat(Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EControlView.GYROSCOPE_Y_AXIS_SENS);
        if(Q3EUtils.q3ei.view_motion_control_gyro && (gyroXSens != 0.0f || gyroYSens != 0.0f))
            mControlGLSurfaceView.SetGyroscopeSens(gyroXSens, gyroYSens);
        mControlGLSurfaceView.RenderView(mGLSurfaceView);

        if(m_portrait)
            InitPortraitGUI();
        else
            InitLandscapeGUI();

        mControlGLSurfaceView.requestFocus();

        m_initView = true;
    }

    private void InitLandscapeGUI()
    {
        RelativeLayout.LayoutParams params;

        params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);

        mainLayout.addView(mGLSurfaceView, params);

        //mControlGLSurfaceView.setZOrderOnTop();
        mControlGLSurfaceView.setZOrderMediaOverlay(true);
        mainLayout.addView(mControlGLSurfaceView, params);

        if(Q3EUtils.q3ei.function_key_toolbar)
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, getResources().getDimensionPixelSize(R.dimen.toolbarHeight));
            View key_toolbar = keyboard.CreateToolbar();
            mainLayout.addView(key_toolbar, params);
            Q3EUtils.SetViewZ(key_toolbar, VIEW_BASE_Z + 3);
        }
        if(Q3EUtils.q3ei.builtin_virtual_keyboard)
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            View vkb = keyboard.CreateBuiltInVKB();
            mainLayout.addView(vkb, params);
            Q3EUtils.SetViewZ(vkb, VIEW_BASE_Z + 2);
        }

        if(m_renderMemStatus > 0) //k
        {
            memoryUsageText = new KDebugTextView(mainLayout.getContext());
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            mainLayout.addView(memoryUsageText, params);
            Q3EUtils.SetViewZ(memoryUsageText, VIEW_BASE_Z + 1);
            memoryUsageText.setTypeface(Typeface.MONOSPACE);
        }
    }

    @SuppressLint("ResourceType")
    private void InitPortraitGUI()
    {
        int[] size = Q3EUtils.GetGeometry(this, true, true, true);

        float ratio = (float) size[3] / (float) size[2];
        int height = (int) ((float) size[3] * ratio);

        RelativeLayout.LayoutParams params;

        params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, height);
        params.topMargin = size[0];
        params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
        params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);

        m_offsetY = params.topMargin;

        mGLSurfaceView.setId(0x20202020);
        mainLayout.addView(mGLSurfaceView, params);

        //mControlGLSurfaceView.setZOrderOnTop();
        mControlGLSurfaceView.setZOrderMediaOverlay(true);
        params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        params.addRule(RelativeLayout.BELOW, mGLSurfaceView.getId());
        //params.addRule(RelativeLayout.ALIGN_PARENT_TOP, RelativeLayout.TRUE);
        params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM, RelativeLayout.TRUE);
        params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
        params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
        mainLayout.addView(mControlGLSurfaceView, params);

        if(Q3EUtils.q3ei.function_key_toolbar)
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, getResources().getDimensionPixelSize(R.dimen.toolbarHeight));
            View key_toolbar = keyboard.CreateToolbar();
            mainLayout.addView(key_toolbar, params);
            Q3EUtils.SetViewZ(key_toolbar, VIEW_BASE_Z + 3);
        }
        if(Q3EUtils.q3ei.builtin_virtual_keyboard)
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            View vkb = keyboard.CreateBuiltInVKB();
            mainLayout.addView(vkb, params);
            Q3EUtils.SetViewZ(vkb, VIEW_BASE_Z + 2);
        }

        if(m_renderMemStatus > 0) //k
        {
            memoryUsageText = new KDebugTextView(mainLayout.getContext());
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            params.addRule(RelativeLayout.BELOW, mGLSurfaceView.getId());
            params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
            mainLayout.addView(memoryUsageText, params);
            Q3EUtils.SetViewZ(memoryUsageText, VIEW_BASE_Z + 1);
            memoryUsageText.setTypeface(Typeface.MONOSPACE);

            memoryUsageText.Start(m_renderMemStatus * 1000);
        }

        keyboard.onAttachedToWindow(m_offsetY);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        return mControlGLSurfaceView.OnKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        return mControlGLSurfaceView.OnKeyUp(keyCode, event);
    }

    private boolean CheckStart()
    {
        // arm32 not support GZDOOM
        if(Q3EUtils.q3ei.isDOOM)
        {
            if(!Q3EJNI.Is64())
            {
                Toast.makeText(this, "GZDOOM not support on arm32 device!", Toast.LENGTH_LONG).show();
                finish();
                Q3EUtils.RunLauncher(this);
                return false;
            }
            String iwad = KidTechCommand.GetParam("-+", Q3EUtils.q3ei.cmd, "iwad");
            if(KStr.IsBlank(iwad))
            {
                Toast.makeText(this, "GZDOOM requires -iwad file!", Toast.LENGTH_LONG).show();
                finish();
                Q3EUtils.RunLauncher(this);
                return false;
            }
        }
        return true;
    }

    private void SetupGame()
    {
        Intent intent = getIntent();
        String intentGame = null;
        String intentCommand = null;
        if(null != intent)
        {
            Bundle extras = intent.getExtras();
            if(null != extras)
            {
                intentGame = extras.getString("game");
                intentCommand = extras.getString("command");
            }
        }
        gameHelper.InitGlobalEnv(intentGame, intentCommand);
    }

    public synchronized void SetupGameViewSize(int width, int height, boolean portrait)
    {
        if(m_portrait == portrait)
        {
            Q3E.GAME_VIEW_WIDTH = width;
            Q3E.GAME_VIEW_HEIGHT = height;
            Q3E.CalcRatio();
        }
    }

    private void MakeMouseCursor()
    {
        if(null == mouseCursor)
        {
            mouseCursor = new KMouseCursor(this);
            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(KMouseCursor.WIDTH, KMouseCursor.HEIGHT);
            mainLayout.addView(mouseCursor, params);
        }
    }

    public void SetMouseCursorVisible(boolean visible)
    {
        if(mControlGLSurfaceView.IsUsingMouse())
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                mControlGLSurfaceView.ShowCursor(visible);
            }
        }
        else
        {
            MakeMouseCursor();
            mouseCursor.SetVisible(visible);
        }
    }

    public void SetMouseCursorPosition(int x, int y)
    {
        if(mControlGLSurfaceView.IsUsingMouse())
            return;
        MakeMouseCursor();
        if(Q3E.IsOriginalSize())
            mouseCursor.SetPosition(x, y + m_offsetY);
        else
        {
            mouseCursor.SetPosition(Q3E.LogicalToPhysicsX(x), Q3E.LogicalToPhysicsY(y) + m_offsetY);
        }
    }

    public Q3EKeyboard GetKeyboard()
    {
        return keyboard;
    }

    public RelativeLayout GetMainLayout()
    {
        return mainLayout;
    }

    public boolean IsCoverEdges()
    {
        return m_coverEdges;
    }

    public boolean IsPortraint()
    {
        return m_portrait;
    }
}
