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
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
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
import com.n0n3m4.q3e.karin.Theme;

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
    private       int            m_offsetX         = 0;
    @SuppressLint("StaticFieldLeak")
    public static Q3EGameHelper gameHelper;
    private MenuItem            editButtonMenu;
    private MenuItem            backMenu;
    private ImageView menuButton;
    private final Handler handler = new Handler();
    private Runnable backCallback;

    private       Q3EKeyboard keyboard;
    private static final int VIEW_BASE_Z = 100;
    private static final int TOOLBAR_Z = VIEW_BASE_Z + 4;
    private static final int VKB_Z = VIEW_BASE_Z + 3;
    private static final int MEM_DEBUG_Z = VIEW_BASE_Z + 2;
    private static final int SETTING_Z = VIEW_BASE_Z + 1;

    private static final float MENU_ICON_ALPHA = 0.5f;
    private static final int MENU_ICON_HIDE_DELAY = 10;

    public final Q3EPermissionRequest permissionRequest = new Q3EPermissionRequest();

    /*
    Intent::extras
    game: game type
    command: command line arguments
     */
    @SuppressLint("ResourceType")
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        KLog.D("UI thread: " + Thread.currentThread().getId());

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

        // setup theme
        Theme.SetTheme(this, false);

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

        if(mGLSurfaceView == null)
            mGLSurfaceView = new Q3EView(this);
        Q3E.gameView = mGLSurfaceView;
        if(mControlGLSurfaceView == null)
            mControlGLSurfaceView = new Q3EControlView(this);
        Q3E.controlView = mControlGLSurfaceView;
        mAudio.vw = mControlGLSurfaceView;

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

        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        int scheme = mPrefs.getInt(Q3EPreference.pref_scrres_scheme, Q3EGlobals.SCREEN_FULL);

        int[] size = Q3EUtils.GetGeometry(this, true, m_hideNav, m_coverEdges);
        if(scheme == Q3EGlobals.SCREEN_FIXED_RATIO)
        {
            int ratioX = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_ratiox, "0"));
            int ratioY = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_ratioy, "0"));

            int width = ViewGroup.LayoutParams.MATCH_PARENT;
            int height = ViewGroup.LayoutParams.MATCH_PARENT;
            int[] sizes = Q3EUtils.CalcSizeByRatio(size[2], size[3], ratioX, ratioY);
            if((sizes[4] & 1) != 0)
            {
                width = sizes[2];
                m_offsetX = sizes[0];
                KLog.i("Q3EView", "Landscape view width-stretch size= %d x %d, X offset=%d", width, size[3], m_offsetX);
            }
            else if((sizes[4] & 2) != 0)
            {
                height = sizes[3];
                m_offsetY = sizes[1];
                KLog.i("Q3EView", "Landscape view height-stretch size= %d x %d, Y offset=%d", size[2], height, m_offsetY);
            }
            else
                KLog.i("Q3EView", "Landscape view stretch size= %d x %d", size[2], size[3]);

            params = new RelativeLayout.LayoutParams(width, height);
            params.addRule(RelativeLayout.CENTER_IN_PARENT, RelativeLayout.TRUE);
        }
        else
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            KLog.i("Q3EView", "Landscape view full size: %d x %d", size[2], size[3]);
        }
        mainLayout.addView(mGLSurfaceView, params);

        params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        //mControlGLSurfaceView.setZOrderOnTop();
        mControlGLSurfaceView.setZOrderMediaOverlay(true);
        mainLayout.addView(mControlGLSurfaceView, params);

        if(Q3EUtils.q3ei.function_key_toolbar)
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, getResources().getDimensionPixelSize(R.dimen.toolbarHeight));
            View key_toolbar = keyboard.CreateToolbar();
            mainLayout.addView(key_toolbar, params);
            Q3EUtils.SetViewZ(key_toolbar, TOOLBAR_Z);
        }
        if(Q3EUtils.q3ei.builtin_virtual_keyboard)
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            View vkb = keyboard.CreateBuiltInVKB();
            mainLayout.addView(vkb, params);
            Q3EUtils.SetViewZ(vkb, VKB_Z);
        }

        if(m_renderMemStatus > 0) //k
        {
            memoryUsageText = new KDebugTextView(mainLayout.getContext());
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            mainLayout.addView(memoryUsageText, params);
            Q3EUtils.SetViewZ(memoryUsageText, MEM_DEBUG_Z);
            memoryUsageText.setTypeface(Typeface.MONOSPACE);
        }
        SetupSettingGate();
    }

    @SuppressLint("ResourceType")
    private void InitPortraitGUI()
    {
        int[] size = Q3EUtils.GetGeometry(this, true, true, true);

        float ratio = (float) size[3] / (float) size[2];
        int avaHeight = (int) ((float) size[3] * ratio);
        int avaWidth = size[3];

        RelativeLayout.LayoutParams params;

        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        int scheme = mPrefs.getInt(Q3EPreference.pref_scrres_scheme, Q3EGlobals.SCREEN_FULL);
        if(scheme == Q3EGlobals.SCREEN_FIXED_RATIO)
        {
            int ratioX = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_ratiox, "0"));
            int ratioY = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_ratioy, "0"));

            int width = ViewGroup.LayoutParams.MATCH_PARENT;
            int height = avaHeight;
            int[] sizes = Q3EUtils.CalcSizeByRatio(avaWidth, avaHeight, ratioX, ratioY);
            if((sizes[4] & 1) != 0)
            {
                width = sizes[2];
                m_offsetX = sizes[0];
                KLog.i("Q3EView", "Portrait view width-stretch size= %d x %d, X offset=%d", width, avaHeight, m_offsetX);
            }
            else if((sizes[4] & 2) != 0)
            {
                height = sizes[3];
                m_offsetY = sizes[1];
                KLog.i("Q3EView", "Portrait view height-stretch size= %d x %d, Y offset=%d", avaWidth, height, m_offsetY);
            }
            else
                KLog.i("Q3EView", "Portrait view stretch size= %d x %d", size[2], size[3]);

            params = new RelativeLayout.LayoutParams(width, height);
            params.topMargin = size[0] + m_offsetY;
            params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
        }
        else
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, avaHeight);

            params.topMargin = size[0];
            params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
            KLog.i("Q3EView", "Portrait view full size: %d x %d", avaWidth, avaHeight);
        }

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
            Q3EUtils.SetViewZ(key_toolbar, TOOLBAR_Z);
        }
        if(Q3EUtils.q3ei.builtin_virtual_keyboard)
        {
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            View vkb = keyboard.CreateBuiltInVKB();
            mainLayout.addView(vkb, params);
            Q3EUtils.SetViewZ(vkb, VKB_Z);
        }

        if(m_renderMemStatus > 0) //k
        {
            memoryUsageText = new KDebugTextView(mainLayout.getContext());
            params = new RelativeLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            params.addRule(RelativeLayout.BELOW, mGLSurfaceView.getId());
            params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
            params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
            mainLayout.addView(memoryUsageText, params);
            Q3EUtils.SetViewZ(memoryUsageText, MEM_DEBUG_Z);
            memoryUsageText.setTypeface(Typeface.MONOSPACE);

            memoryUsageText.Start(m_renderMemStatus * 1000);
        }
        SetupSettingGate();

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
        if(Q3EUtils.q3ei.IsDisabled()) // disabled or removed games
        {
            Toast.makeText(this, Q3EUtils.q3ei.game_name + " is disabled or removed!", Toast.LENGTH_LONG).show();
            finish();
            Q3EUtils.RunLauncher(this);
            return false;
        }
        else if(Q3EUtils.q3ei.isDOOM) // arm32 not support UZDOOM
        {
            if(!Q3EJNI.Is64())
            {
                Toast.makeText(this, "UZDOOM not support on arm32 device!", Toast.LENGTH_LONG).show();
                finish();
                Q3EUtils.RunLauncher(this);
                return false;
            }
            String iwad = KidTechCommand.GetParam("-+", Q3EUtils.q3ei.cmd, "iwad");
            if(KStr.IsBlank(iwad))
            {
                Toast.makeText(this, "UZDOOM requires -iwad file!", Toast.LENGTH_LONG).show();
                finish();
                Q3EUtils.RunLauncher(this);
                return false;
            }
        }
        else if(Q3EGlobals.IsFDroidVersion())
        {
            if(Q3EUtils.q3ei.isXash3D)
            {
                Toast.makeText(this, "F-Droid version not support Xash3D, you can install Github version!", Toast.LENGTH_LONG).show();
                finish();
                Q3EUtils.RunLauncher(this);
                return false;
            }
            else if(Q3EUtils.q3ei.isSource)
            {
                Toast.makeText(this, "F-Droid version not support Source-Engine game, you can install Github version!", Toast.LENGTH_LONG).show();
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
        //if(m_portrait == portrait)
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
            mouseCursor.SetPosition(x + m_offsetX, y + m_offsetY);
        else
        {
            mouseCursor.SetPosition(Q3E.LogicalToPhysicsX(x) + m_offsetX, Q3E.LogicalToPhysicsY(y) + m_offsetY);
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

    public boolean IsPortrait()
    {
        return m_portrait;
    }

    public void RequestPermission(String permission, int requestCode) {
        permissionRequest.Request(permission, requestCode);

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M/* 23 *//* Android 6.0 (M) */) {
            KLog.I("Native request permission: " + permission + " with request code " +  requestCode + " -> " + true);
            synchronized(permissionRequest) {
                permissionRequest.Result(true);
                permissionRequest.notifyAll();
            }
            return;
        }

        if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{ permission }, requestCode);
        } else {
            KLog.I("Native request permission: " + permission + " with request code " +  requestCode + " has granted");
            synchronized(permissionRequest) {
                permissionRequest.Result(true);
                permissionRequest.notifyAll();
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        if(permissionRequest.requestCode == requestCode && grantResults.length > 0)
        {
            boolean result = (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED);
            KLog.I("Native request permission: " + permissions[0] + " with request code " +  requestCode + " -> " + result);
            synchronized(permissionRequest) {
                permissionRequest.Result(result);
                permissionRequest.notifyAll();
            }
        }
    }

    private void SetupSettingGate()
    {
        SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        if(mPrefs.getBoolean(Q3EPreference.pref_hideonscr, false))
            return;

        int px = Q3EUtils.dip2px(this, 48);
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(px, px);
        params.addRule(RelativeLayout.ALIGN_PARENT_TOP | RelativeLayout.CENTER_HORIZONTAL);
        if(m_portrait)
        {
            params.addRule(RelativeLayout.BELOW, mGLSurfaceView.getId());
            //params.topMargin = m_offsetY;
        }

        menuButton = new ImageView(this);
        menuButton.setAlpha(MENU_ICON_ALPHA);
        menuButton.setFocusable(false);
        menuButton.setFocusableInTouchMode(false);
        menuButton.setImageDrawable(getResources().getDrawable(R.drawable.icon_m_settings));
        mainLayout.addView(menuButton, params);
        Q3EUtils.SetViewZ(menuButton, SETTING_Z);
        menuButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                OpenMenu();
            }
        });
        ShowMenuIcon(0.6f, 30);
    }

    private void OpenMenu()
    {
        ShowMenuIcon(-1.0f, -1);
        openOptionsMenu();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.main_menu, menu);

        editButtonMenu = menu.findItem(R.id.main_edit_button_layout);
        backMenu = menu.findItem(R.id.main_quit);
        if(m_portrait)
        {
            menu.findItem(R.id.main_edit_button_layout).setVisible(false);
        }
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        if(preferences.getBoolean(Q3EPreference.BUILTIN_VIRTUAL_KEYBOARD, false))
            menu.findItem(R.id.main_choose_input_method).setVisible(false);

        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int itemId = item.getItemId();
        if (itemId == R.id.main_edit_button_layout)
        {
            ToggleButtonEditor();
            return true;
        }
        else if (itemId == R.id.main_choose_input_method)
        {
            Q3EUtils.ChooseInputMethod(this);
            return true;
        }
        else if (itemId == R.id.main_quit)
        {
            if(null != backCallback)
            {
                backCallback.run();
                backCallback = null;
            }
            else
                Q3E.activity.Quit();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void ToggleButtonEditor()
    {
        boolean editMode = mControlGLSurfaceView.IsEditMode();

        mControlGLSurfaceView.ToggleMode(new Runnable() {
            @Override
            public void run() {
                ShowMenuIcon(-1.0f, -1);
                backCallback = null;
                UpdateMenu();
            }
        }, new Runnable() {
            @Override
            public void run() {
                ShowMenuIcon(1.0f, 0);
                UpdateMenu();
            }
        });

        if(!editMode)
            backCallback = new Runnable() {
                @Override
                public void run() {
                    mControlGLSurfaceView.ExitEditMode(false);
                }
            };
        else
            backCallback = null;
    }

    private final Runnable hideMenuIcon_f = new Runnable() {
        @Override
        public void run()
        {
            if(null != menuButton)
                menuButton.setAlpha(0.0f);
        }
    };

    private void ShowMenuIcon(float alpha, int autoHide)
    {
        handler.removeCallbacks(hideMenuIcon_f);
        if(null != menuButton)
        {
            menuButton.setAlpha(alpha < 0.0f ? MENU_ICON_ALPHA : alpha);
            if(autoHide > 0)
                handler.postDelayed(hideMenuIcon_f, autoHide * 1000L);
            else if(autoHide < 0)
                handler.postDelayed(hideMenuIcon_f, MENU_ICON_HIDE_DELAY * 1000L);
        }
    }

    private void UpdateMenu()
    {
        if(null != mControlGLSurfaceView && mControlGLSurfaceView.IsEditMode())
            editButtonMenu.setTitle(R.string.finish_editing);
        else
            editButtonMenu.setTitle(R.string.edit_buttons_layout);

        if(null != backCallback)
            backMenu.setTitle(R.string.back);
        else
            backMenu.setTitle(R.string.quit);
    }

    public void Quit()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.exit_game);
        builder.setMessage(R.string.are_you_sure_exit_game);
        builder.setCancelable(true);
        builder.setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int v)
            {
                dialog.dismiss();
                Q3E.Shutdown();
            }
        });
        builder.setNegativeButton(R.string.cancel, null);
        builder.create().show();
    }
}
